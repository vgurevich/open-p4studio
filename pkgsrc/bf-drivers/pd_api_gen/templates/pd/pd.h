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

#ifndef _PD_${p4_name.upper()}_PD_H
#define _PD_${p4_name.upper()}_PD_H

#include <stdint.h>

#if __BYTE_ORDER == __LITTLE_ENDIAN_
#define LITTLE_ENDIAN_CALLER 1
#endif

#include <tofino/pdfixed/pd_common.h>
//:: if gen_md_pd:
#include <tofino/pdfixed/pd_plcmt.h>
//:: #endif
#include <pipe_mgr/pipe_mgr_intf.h>


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
//:: def gen_action_params(names, bit_widths, _get_type = get_type):
//::   params = []
//::   for name, width in zip(names, bit_widths):
//::     name = "action_" + name
//::     width = (width + 7) // 8
//::     params += [(name, width)]
//::   #endfor
//::   return params
//:: #enddef
//::
//::

/* MATCH STRUCTS */

//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]:
/* ${table} has no match fields */

//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
typedef struct ${p4_pd_prefix}${table}_match_spec {
//::   for name, width in match_params:
//::     if width > 4:
  uint8_t ${name}[${width}];
//::     else:
//::       type_ = get_type(width)
  ${type_} ${name};
//::     #endif
//::   #endfor
} ${p4_pd_prefix}${table}_match_spec_t;

//:: #endfor


/* Dynamic Exm Table Key Mask */

//:: for table, t_info in table_info.items():
//::   if not t_info["match_fields"]:
/* ${table} has no match fields */

//::     continue
//::   #endif
//::   if not t_info["dynamic_match_key_masks"]:
/* ${table} has no dynamic key masks */

//::     continue
//::   #endif
//::   match_params = gen_match_params(t_info["match_fields"], field_info)
typedef struct ${p4_pd_prefix}${table}_match_key_mask_spec {
//::   for name, width in match_params:
//::     name = name + '_mask'
//::     if width > 4:
  uint8_t ${name}[${width}];
//::     else:
//::       type_ = get_type(width)
  ${type_} ${name};
//::     #endif
//::   #endfor
} ${p4_pd_prefix}${table}_match_key_mask_spec_t;

//:: #endfor


/* ACTION STRUCTS */

/* Enum of all action names. */
typedef enum ${p4_pd_prefix}action_names {
//:: for action, a_info in action_info.items():
  ${p4_pd_prefix}${action},
//:: #endfor
  ${p4_pd_prefix}action_names_t_invalid
} ${p4_pd_prefix}action_names_t;

const char* ${p4_pd_prefix}action_enum_to_string(${p4_pd_prefix}action_names_t e);

${p4_pd_prefix}action_names_t ${p4_pd_prefix}action_string_to_enum(const char* s);

//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]:
  /* ${action} has no parameters */

//::     continue
//::   #endif
//::   action_params = gen_action_params(a_info["param_names"],
//::                                     a_info["param_bit_widths"])
typedef struct ${p4_pd_prefix}${action}_action_spec {
//::   for name, width in action_params:
//::     if width > 4:
  uint8_t ${name}[${width}];
//::     else:
//::       type_ = get_type(width)
  ${type_} ${name};
//::     #endif
//::   #endfor
} ${p4_pd_prefix}${action}_action_spec_t;

//:: #endfor

typedef struct ${p4_pd_prefix}action_specs_t {
  ${p4_pd_prefix}action_names_t name;
  union {
//:: for action, a_info in action_info.items():
//::   if not a_info["param_names"]:
  /* ${action} has no parameters */
//::     continue
//::   #endif
    struct ${p4_pd_prefix}${action}_action_spec ${p4_pd_prefix}${action};
//:: #endfor
  } u;
} ${p4_pd_prefix}action_specs_t;

void ${p4_pd_prefix}init(void);

/* HA TESTING INFRASTRUCTURE */
//:: if gen_hitless_ha_test_pd:
typedef struct ${p4_pd_prefix}ha_reconc_report_t {
  uint32_t num_entries_added;
  uint32_t num_entries_deleted;
  uint32_t num_entries_modified;
} ${p4_pd_prefix}ha_reconc_report_t;
//:: #endif

/* REGISTER VALUES */

//:: for register, r_info in register_info.items():
//::   if r_info["layout"]:
typedef struct ${p4_pd_prefix}${register}_value {
//::     for f_name, f_width in r_info["layout"]:
  ${get_reg_type(f_width)} ${f_name};
//::     #endfor
} ${p4_pd_prefix}${register}_value_t;

//::   #endif
//:: #endfor

/* IDLE TIME CONFIG */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not t_info["timeout"]: continue
//::   name = p4_pd_prefix + table + "_idle_tmo_enable"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_idle_time_params_t params
);

//::   name = p4_pd_prefix + table + "_idle_register_tmo_cb"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_idle_tmo_expiry_cb cb,
 void *cookie
);

//::   name = p4_pd_prefix + table + "_idle_tmo_disable"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id
);

//::   name = p4_pd_prefix + table + "_idle_params_get"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_idle_time_params_t *params
);

//::   name = p4_pd_prefix + table + "_set_ttl"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_entry_hdl_t entry_hdl,
 uint32_t ttl
);

//::   name = p4_pd_prefix + table + "_get_ttl"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_entry_hdl_t entry_hdl,
 uint32_t *ttl
);

//::   name = p4_pd_prefix + table + "_reset_ttl"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_entry_hdl_t entry_hdl
);

//::   name = p4_pd_prefix + table + "_update_hit_state"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_idle_time_update_complete_cb callback_fn,
 void *cookie
);

//::   name = p4_pd_prefix + table + "_get_hit_state"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_entry_hdl_t entry_hdl,
 p4_pd_idle_time_hit_state_e *hit_state
);

//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if len(t_info["match_fields"]) == 0:
//::     continue
//::   #endif
//::   match_type = t_info["match_type"]
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::              "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     params += ["int priority"]
//::   #endif
//::   params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + table + "_match_spec_to_entry_hdl"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor


/* Dynamic Exm Table Key Mask */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if len(t_info["match_fields"]) == 0:
//::     continue
//::   #endif
//::   if not t_info["dynamic_match_key_masks"]:
//::     continue
//::   #endif
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::              p4_pd_prefix + table + "_match_key_mask_spec_t *match_key_mask_spec"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + table + "_match_key_mask_spec_set"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::              p4_pd_prefix + table + "_match_key_mask_spec_t *match_key_mask_spec"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + table + "_match_key_mask_spec_get"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + table + "_match_key_mask_spec_reset"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

/* ADD ENTRIES */

//::
//:: def get_direct_parameter_specs(direct_resources, register_info):
//::   specs = []
//::   for res_name, res_type, _ in direct_resources:
//::     if res_type == "bytes_meter":
//::       specs += ["p4_pd_bytes_meter_spec_t *" + res_name + "_spec"]
//::     elif res_type == "packets_meter":
//::       specs += ["p4_pd_packets_meter_spec_t *" + res_name + "_spec"]
//::     elif res_type == "register":
//::       specs += [register_info[res_name]["v_type"] + " *" + res_name + "_spec"]
//::     elif res_type == "lpf":
//::       specs += ["p4_pd_lpf_spec_t *" + res_name + "_spec"]
//::     elif res_type == "wred":
//::       specs += ["p4_pd_wred_spec_t *" + res_name + "_spec"]
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
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt",
//::               "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::     if match_type == "ternary" or match_type == "range":
//::       params += ["int priority"]
//::     #endif
//::     if has_action_spec:
//::       params += ["const " + p4_pd_prefix + action + "_action_spec_t * const action_spec"]
//::     #endif
//::     if t_info["timeout"]:
//::       params += ["uint32_t ttl"]
//::     #endif
//::     params += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + table + "_table_add_with_" + action
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
 * @param match_spec
//::     if match_type == "ternary" or match_type == "range":
 * @param priority
//::     #endif
//::     if has_action_spec:
 * @param action_spec
//::     #endif
 * @param entry_hdl
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor

/* DELETE ENTRIES */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = p4_pd_prefix + table + "_table_delete"
//::     params = ["p4_pd_sess_hdl_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["p4_pd_dev_target_t dev_tgt",
//::                  "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["int priority"]
//::       #endif
//::     else:
//::       params += ["bf_dev_id_t dev_id",
//::                  "p4_pd_entry_hdl_t entry_hdl"]
//::     #endif
//::     param_str = ",\n ".join(params)
/**
 * @brief ${name}
 * @param sess_hdl
//::     if idx:
 * @param dev_tgt
 * @param match_spec
//::       if match_type == "ternary" or match_type == "range":
 * @param priority
//::       #endif
//::     else:
 * @param dev_id
 * @param entry_hdl
//::     #endif
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor

/* Get default entry handle */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = p4_pd_prefix + table + "_table_get_default_entry_handle"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t pd_dev_tgt,
 p4_pd_entry_hdl_t* entry_hdl
);

//::   name = p4_pd_prefix + table + "_table_get_default_entry"
//::   common_params = ["p4_pd_sess_hdl_t sess_hdl",
//::                    "p4_pd_dev_target_t pd_dev_tgt",
//::                    "bool read_from_hw"]
//::   if not action_table_hdl:
//::     params = common_params + [p4_pd_prefix + "action_specs_t * const action_spec"]
//::   else:
//::     # with indirect action data
//::     act_prof = t_info["action_profile"]
//::     assert(act_prof is not None)
//::
//::     if select_hdl:
//::       params = common_params + ["bool *has_mbr_hdl, bool *has_grp_hdl, p4_pd_grp_hdl_t *grp_hdl, p4_pd_mbr_hdl_t *mbr_hdl"]
//::     else:
//::       params = common_params + ["p4_pd_mbr_hdl_t *mbr_hdl"]
//::     #endif
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       bound_indirect_res += [indirect_res_name]
//::     #endfor
//::     params += ["int *" + r + "_index" for r in bound_indirect_res]
//::   #endif
//::   param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

/* Clear default entry */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = p4_pd_prefix + table + "_table_reset_default_entry"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t pd_dev_tgt
);

//:: #endfor

/* MODIFY TABLE PROPERTIES */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   name = "p4_pd_" + p4_prefix + "_" + table + "_set_property"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_tbl_prop_type_t property,
 p4_pd_tbl_prop_value_t value,
 p4_pd_tbl_prop_args_t args
);

//::   name = "p4_pd_" + p4_prefix + "_" + table + "_get_property"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_tbl_prop_type_t property,
 p4_pd_tbl_prop_value_t *value,
 p4_pd_tbl_prop_args_t *args
);

//:: #endfor

/* MODIFY ENTRIES */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for action in t_info["actions"]:
//::     a_info = action_info[action]
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     for idx in range(2):
//::       name = p4_pd_prefix + table + "_table_modify_with_" + action
//::       params = ["p4_pd_sess_hdl_t sess_hdl"]
//::       if idx:
//::         name += "_by_match_spec"
//::         params += ["p4_pd_dev_target_t dev_tgt",
//::                    "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::         if match_type == "ternary" or match_type == "range":
//::           params += ["int priority"]
//::         #endif
//::       else:
//::         params += ["bf_dev_id_t dev_id",
//::                    "p4_pd_entry_hdl_t ent_hdl"]
//::       #endif
//::       if has_action_spec:
//::         params += ["const " + p4_pd_prefix + action + "_action_spec_t * const action_spec"]
//::       #endif
//::       params += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::       param_str = ",\n ".join(params)
/**
 * @brief ${name}
 * @param sess_hdl
//::       if idx:
 * @param dev_tgt
 * @param match_spec
//::         if match_type == "ternary" or match_type == "range":
 * @param priority
//::         #endif
//::       else:
 * @param dev_id
 * @param entry_hdl
//::       #endif
//::       if has_action_spec:
 * @param action_spec
//::       #endif
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::     #endfor
//::   #endfor
//:: #endfor


/* SET DEFAULT_ACTION */

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if action_table_hdl: continue
//::   for action in t_info["actions"]:
//::     name = "p4_pd_" + p4_prefix + "_" + table + "_set_default_action_" + action
//::     a_info = action_info[action]
//::     if not a_info['allowed_to_be_default_action'][table]: continue
//::     has_action_spec = len(a_info["param_names"]) > 0
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += ["const " + p4_pd_prefix + action + "_action_spec_t * const action_spec"]
//::     #endif
//::     params += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::     params += ["p4_pd_entry_hdl_t *entry_hdl"]
//::     param_str = ",\n ".join(params)
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
//::     if has_action_spec:
 * @param action_spec
//::     #endif
 * @param entry_hdl
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor


/* INDIRECT ACTION DATA AND MATCH SELECT */

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
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     if has_action_spec:
//::       params += ["const " + p4_pd_prefix + action + "_action_spec_t * const action_spec"]
//::     #endif
//::     params += ["p4_pd_mbr_hdl_t *mbr_hdl"]
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + act_prof + "_add_member_with_" + action
p4_pd_status_t
${name}
(
 ${param_str}
);

//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::     "bf_dev_id_t dev_id",
//::     "p4_pd_mbr_hdl_t mbr_hdl"]
//::     if has_action_spec:
//::       params += ["const " + p4_pd_prefix + action + "_action_spec_t * const action_spec"]
//::     #endif
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + act_prof + "_modify_member_with_" + action
p4_pd_status_t
${name}
(
 ${param_str}
);
//::   name = p4_pd_prefix + act_prof + "_set_property"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_tbl_prop_type_t property,
 p4_pd_tbl_prop_value_t value
);
//::   #endfor
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_member"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   if not select_hdl: continue
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_sel_tbl_update_cb cb",
//::             "void *cb_ctx"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_register_callback"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::             "uint32_t max_grp_size",
//::             "p4_pd_grp_hdl_t *grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_create_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_add_member_to_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_del_member_from_group"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl",
//::             "enum p4_pd_grp_mbr_state_e mbr_state"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_group_member_state_set"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "p4_pd_mbr_hdl_t mbr_hdl",
//::             "enum p4_pd_grp_mbr_state_e *mbr_state_p"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_group_member_state_get"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_grp_hdl_t grp_hdl",
//::             "uint8_t *hash",
//::             "uint32_t hash_len",
//::             "p4_pd_mbr_hdl_t *mbr_hdl_p"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_group_member_get_from_hash"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::             "p4_pd_mbr_hdl_t mbr_hdl"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_set_dynamic_action_selection_fallback_member"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_reset_dynamic_action_selection_fallback_member"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   params = ["bf_dev_id_t dev_id",
//::             "int (*cb_func)(p4_pd_sess_hdl_t, p4_pd_dev_target_t, void*, unsigned int, unsigned int, int, bool)",
//::             "void *cookie"]
//::   param_str = ",\n ".join(params)
//::   name = p4_pd_prefix + act_prof + "_sel_track_updates"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt",
//::             "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::   if match_type == "ternary" or match_type == "range":
//::     params += ["int priority"]
//::   #endif
//::   params_wo = params + ["p4_pd_mbr_hdl_t mbr_hdl"]
//::   if t_info["timeout"]:
//::     params_wo += ["uint32_t ttl"]
//::   #endif
//::   params_wo += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::   bound_indirect_res = []
//::   for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::     bound_indirect_res += [indirect_res_name]
//::   #endfor
//::   params_wo += ["int " + r + "_index" for r in bound_indirect_res]
//::   params_wo += ["p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_wo)
//::   name = p4_pd_prefix + table + "_add_entry"
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   if not select_hdl: continue
//::   params_w = params + ["p4_pd_grp_hdl_t grp_hdl"]
//::   if t_info["timeout"]:
//::     params_w += ["uint32_t ttl"]
//::   #endif
//::   params_w += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::   params_w += ["int " + r + "_index" for r in bound_indirect_res]
//::   params_w += ["p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_w)
//::   name = p4_pd_prefix + table + "_add_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor


//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = p4_pd_prefix + table + "_modify_entry"
//::     params = ["p4_pd_sess_hdl_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["p4_pd_dev_target_t dev_tgt",
//::                  "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["int priority"]
//::       #endif
//::     else:
//::       params += ["bf_dev_id_t dev_id",
//::                  "p4_pd_entry_hdl_t entry_hdl"]
//::     #endif
//::     params_wo = params + ["p4_pd_mbr_hdl_t mbr_hdl"]
//::     params_wo += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       bound_indirect_res += [indirect_res_name]
//::     #endfor
//::     params_wo += ["int " + r + "_index" for r in bound_indirect_res]
//::     param_str = ",\n ".join(params_wo)
p4_pd_status_t
${name}
(
 ${param_str}
);

//::     if not select_hdl: continue
//::     params_w = params + ["p4_pd_grp_hdl_t grp_hdl"]
//::     params_w += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::     params_w += ["int " + r + "_index" for r in bound_indirect_res]
//::     param_str = ",\n ".join(params_w)
//::     name = p4_pd_prefix + table + "_modify_entry_with_selector"
//::     if idx:
//::       name += "_by_match_spec"
//::     #endif
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor


//:: for table, t_info in table_info.items():
//::   name = p4_pd_prefix + table + "_get_entry_count"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 uint32_t *count
);

//:: #endfor

//:: action_profiles = set()
//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   act_prof = t_info["action_profile"]
//::   assert(act_prof is not None)
//::   if act_prof in action_profiles: continue
//::   action_profiles.add(act_prof)
//::   name = p4_pd_prefix + act_prof + "_get_act_prof_entry_count"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 uint32_t *count
);

//::   if not select_hdl: continue
//::   name = p4_pd_prefix + act_prof + "_get_selector_group_count"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 uint32_t *count
);

//::   name = p4_pd_prefix + act_prof + "_get_selector_group_member_count"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_grp_hdl_t grp_hdl,
 uint32_t *count
);

//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   match_type = t_info["match_type"]
//::   has_match_spec = len(t_info["match_fields"]) > 0
//::   name = p4_pd_prefix + table + "_get_first_entry_handle"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t *entry_handle
);

//::   name = p4_pd_prefix + table + "_get_next_entry_handles"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_handle,
 int n,
 p4_pd_entry_hdl_t *next_entry_handles
);

//::   name = p4_pd_prefix + table + "_get_entry"
//::   common_params = ["p4_pd_sess_hdl_t sess_hdl",
//::                    "bf_dev_id_t dev_id",
//::                    "p4_pd_entry_hdl_t entry_hdl",
//::                    "bool read_from_hw"]
//::   if has_match_spec:
//::     common_params += [p4_pd_prefix + table + "_match_spec_t * match_spec"]
//::   #endif
//::   if match_type == "ternary" or match_type == "range":
//::     common_params += ["int *priority"]
//::   #endif
//::
//::   if not action_table_hdl:
//::     params = common_params + [p4_pd_prefix + "action_specs_t * const action_spec"]
//::     param_str = ",\n ".join(params)
//::   else:
//::     # with indirect action data
//::     act_prof = t_info["action_profile"]
//::     assert(act_prof is not None)
//::
//::     if select_hdl:
//::       params = common_params + ["bool *has_mbr_hdl, bool *has_grp_hdl, p4_pd_grp_hdl_t *grp_hdl, p4_pd_mbr_hdl_t *mbr_hdl"]
//::     else:
//::       params = common_params + ["p4_pd_mbr_hdl_t *mbr_hdl"]
//::     #endif
//::     bound_indirect_res = []
//::     for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::       bound_indirect_res += [indirect_res_name]
//::     #endfor
//::     params += ["int *" + r + "_index" for r in bound_indirect_res]
//::     param_str = ",\n ".join(params)
//::   #endif
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   if gen_md_pd:
//::     name = p4_pd_prefix + table + "_get_entry_from_plcmt_data"
//::     common_params = ["void *mat_plcmt_data"]
//::     if has_match_spec:
//::       common_params += [p4_pd_prefix + table + "_match_spec_t *match_spec"]
//::     #endif
//::     if match_type == "ternary" or match_type == "range":
//::       common_params += ["int *priority"]
//::     #endif
//::     if not action_table_hdl:
//::       params = common_params + [p4_pd_prefix + "action_specs_t * const action_spec"]
//::     else:
//::       act_prof = t_info["action_profile"]
//::       assert(act_prof is not None)
//::       params = common_params + ["p4_pd_mbr_hdl_t *mbr_hdl",
//::                                 "p4_pd_grp_hdl_t *grp_hdl",
//::                                 "bool *has_mbr_hdl",
//::                                 "bool *has_grp_hdl"]
//::     #endif
//::     param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);

//::     name = p4_pd_prefix + table + "_get_plcmt_data"
//::     params = ["p4_pd_sess_hdl_t sess_hdl", "bf_dev_id_t dev_id"]
//::     param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);

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
//::   name = p4_pd_prefix + act_prof + "_get_member"
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl",
//::             "bool read_from_hw",
//::             p4_pd_prefix + "action_specs_t *action_spec"]
//::   param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);
//::   if gen_md_pd:

//::   name = p4_pd_prefix + act_prof + "_get_member_from_plcmt_data"
//::   params = ["void *adt_plcmt_data",
//::             p4_pd_prefix + "action_specs_t * const action_spec"]
//::   param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   name = p4_pd_prefix + act_prof + "_get_plcmt_data"
//::   params = ["p4_pd_sess_hdl_t sess_hdl", "bf_dev_id_t dev_id"]
//::   param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   name = p4_pd_prefix + act_prof + "_get_full_member_info_from_plcmt_data"
//::   params = ["uint8_t device_id",
//::             "p4_pd_mbr_hdl_t mbr_hdl",
//::             "void *adt_plcmt_data",
//::             p4_pd_prefix + "action_specs_t * const action_spec"]
//::   param_str = ",\n ".join(params)
/* This API is similar to the api above, with the exception that
 * it will return the complete member info, complete with indirect
 * resource indices as well. This API takes in the device id and the
 * mbr hdl as arguments for that purpose.
 */
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endif
//::     name = p4_pd_prefix + act_prof + "_get_first_member"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, int *entry_handle
);

//::     name = p4_pd_prefix + act_prof + "_get_next_members"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, p4_pd_entry_hdl_t entry_handle,
  int n, int *next_entry_handles
);

//::   if not select_hdl: continue
//::   name = p4_pd_prefix + act_prof + "_get_first_group"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, int *grp_hdl
);

//::   name = p4_pd_prefix + act_prof + "_get_next_groups"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, int grp_hdl,
  int n, int *grp_hdls
);

//::   name = p4_pd_prefix + act_prof + "_get_first_group_member"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, int grp_hdl,
  int *mbr_hdl
);

//::   name = p4_pd_prefix + act_prof + "_get_next_group_members"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, bf_dev_id_t dev_id, int grp_hdl,
  int mbr_hdl, int n, int *mbr_hdls
);

//::   if gen_md_pd:
//::     name = p4_pd_prefix + act_prof + "_get_word_llp_active_member_count"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, int word_index,
  int *count
);

//::     name = p4_pd_prefix + act_prof + "_get_word_llp_active_members"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, p4_pd_dev_target_t dev_tgt, int word_index,
  int count, int *mbr_hdls
);

//::     name = p4_pd_prefix + act_prof + "_sel_get_plcmt_data"
p4_pd_status_t
${name}
(
  p4_pd_sess_hdl_t sess_hdl, bf_dev_id_t dev_id
);
//::   #endif
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   match_type = t_info["match_type"]
//::   if len(t_info["match_fields"]) == 0: continue
//::   for idx in range(2):
//::     name = p4_pd_prefix + table + "_table_delete"
//::     params = ["p4_pd_sess_hdl_t sess_hdl"]
//::     if idx:
//::       name += "_by_match_spec"
//::       params += ["p4_pd_dev_target_t dev_tgt",
//::                  "const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["int priority"]
//::       #endif
//::     else:
//::       params += ["bf_dev_id_t dev_id",
//::                  "p4_pd_entry_hdl_t entry_hdl"]
//::     #endif
//::     param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endfor

//:: for table, t_info in table_info.items():
//::   table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::   if not action_table_hdl: continue
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   params_wo = params + ["p4_pd_mbr_hdl_t mbr_hdl"]
//::   params_wo += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::   bound_indirect_res = []
//::   for indirect_res_name in t_info["ap_bind_indirect_res_to_match"]:
//::     bound_indirect_res += [indirect_res_name]
//::   #endfor
//::   params_wo += ["int " + r + "_index" for r in bound_indirect_res]
//::   params_wo += ["p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_wo)
//::   name = p4_pd_prefix + table + "_set_default_entry"
p4_pd_status_t
${name}
(
 ${param_str}
);


//::   if not select_hdl: continue
//::   params_w = params + ["p4_pd_grp_hdl_t grp_hdl"]
//::   params_w += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::   params_w += ["int " + r + "_index" for r in bound_indirect_res]
//::   params_w += ["p4_pd_entry_hdl_t *entry_hdl"]
//::   param_str = ",\n ".join(params_w)
//::   name = p4_pd_prefix + table + "_set_default_entry_with_selector"
p4_pd_status_t
${name}
(
 ${param_str}
);
//:: #endfor

//:: for lq in learn_quanta:
typedef struct __attribute__((__packed__)) ${p4_pd_prefix}${lq["name"]}_digest_entry {
//:: for field in lq["fields"]:
//::   field_name = field["field_name"]
//::   byte_width = ( field_info[field_name]["bit_width"] + 7 ) // 8
//::   if byte_width > 4:
  uint8_t ${field_name}[${byte_width}];
//::   else:
//::     type = get_type(byte_width)
  ${type} ${field_name};
//::   #endif
//:: #endfor
} ${lq["entry_type"]};

// Should be able to cast this to pipe_flow_lrn_msg_t.
typedef struct ${p4_pd_prefix}${lq["name"]}_digest_msg {
  p4_pd_dev_target_t      dev_tgt;
  uint16_t                num_entries;
  ${lq["entry_type"]}    *entries;
  uint32_t                flow_lrn_fld_lst_hdl;
} ${lq["msg_type"]};

// Should be able to cast this to pipe_flow_lrn_notify_cb.
typedef p4_pd_status_t (*${lq["cb_fn_type"]})(p4_pd_sess_hdl_t sess_hdl,
                                              ${lq["msg_type"]} *msg,
                                              void *callback_fn_cookie);

p4_pd_status_t
${lq["register_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 uint8_t                  device_id,
 ${lq["cb_fn_type"]}      cb_fn,
 void                    *cb_fn_cookie
);

p4_pd_status_t
${lq["deregister_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 uint8_t                  device_id
);

p4_pd_status_t
${lq["notify_ack_fn"]}
(
 p4_pd_sess_hdl_t         sess_hdl,
 ${lq["msg_type"]}        *msg
);
//:: #endfor

p4_pd_status_t
${p4_pd_prefix}set_learning_timeout(p4_pd_sess_hdl_t shdl,
                                    uint8_t          device_id,
                                    uint32_t         usecs);

/* COUNTERS */

//:: for counter, c_info in counter_info.items():
//::   binding = c_info["binding"]
//::   type_ = c_info["type_"]
//::   if binding[0] == "direct":
//::     name = "p4_pd_" + p4_prefix + "_counter_read_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_hdl,
 int flags,
 p4_pd_counter_value_t *counter_value
);

//::     name = "p4_pd_" + p4_prefix + "_counter_write_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_hdl,
 p4_pd_counter_value_t counter_value
);

//::   else:
//::     name = "p4_pd_" + p4_prefix + "_counter_read_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 int flags,
 p4_pd_counter_value_t *counter_value
);

//::     name = "p4_pd_" + p4_prefix + "_counter_write_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 p4_pd_counter_value_t counter_value
);

//::   #endif
//:: #endfor

//:: for counter, c_info in counter_info.items():
//::   name = "p4_pd_" + p4_prefix + "_counter_hw_sync_" + counter
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_stat_sync_cb cb_fn,
 void *cb_cookie
);

//:: #endfor


// REGISTERS

//:: for register, r_info in register_info.items():
//::   binding = r_info["binding"]
//::   table_type = r_info["table_type"]
//::   if table_type == "fifo":
//::     name = "p4_pd_" + p4_prefix + "_register_occupancy_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int *occupancy
);

//::     name = "p4_pd_" + p4_prefix + "_register_reset_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
);

//::     if r_info['direction'] == "in":
//::       name = "p4_pd_" + p4_prefix + "_register_dequeue_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int num_to_dequeue,
 ${r_info["v_type"]} *register_values,
 int *num_dequeued
);

//::     #endif
//::     if r_info['direction'] == "out":
//::       name = "p4_pd_" + p4_prefix + "_register_enqueue_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int num_to_enqueue,
 ${r_info["v_type"]} *register_values
);

//::     #endif
//::   else:
//::     name = "p4_pd_" + p4_prefix + "_register_hw_sync_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_register_sync_cb cb_fn,
 void *cb_cookie
);

//::     if binding[0] == "direct":
//::       name = "p4_pd_" + p4_prefix + "_register_read_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 p4_pd_entry_hdl_t entry_hdl,
 int flags,
 ${r_info["v_type"]} *register_values,
 int *value_count
);

//::       name = "p4_pd_" + p4_prefix + "_register_write_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 bf_dev_id_t dev_id,
 p4_pd_entry_hdl_t entry_hdl,
 ${r_info["v_type"]} *register_value
);

//::     else:
//::       name = "p4_pd_" + p4_prefix + "_register_read_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 int flags,
 ${r_info["v_type"]} *register_values,
 int *value_count
);

//::       name = "p4_pd_" + p4_prefix + "_register_write_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 ${r_info["v_type"]} *register_value
);

//::       name = "p4_pd_" + p4_prefix + "_register_reset_all_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
);

//::       name = "p4_pd_" + p4_prefix + "_register_range_reset_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 int count
);

//::       name = "p4_pd_" + p4_prefix + "_register_write_all_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 ${r_info["v_type"]} *register_value
);

//::       name = "p4_pd_" + p4_prefix + "_register_range_read_" + register
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt,
 int index,
 int count,
 int flags,
 int *num_actually_read,
 ${r_info["v_type"]} *register_values,
 int *value_count
);
//::     #endif
//::   #endif

//:: #endfor


/* METERS */

//:: for meter, m_info in meter_info.items():
//::   binding = m_info["binding"]
//::   type_ = m_info["type_"]
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     params += ["int index"]
//::   #endif
//::   if type_ == "packets":
//::     params += ["p4_pd_packets_meter_spec_t *meter_spec"]
//::   else:
//::     params += ["p4_pd_bytes_meter_spec_t *meter_spec"]
//::   #endif
//::   param_str = ",\n ".join(params)
//::   name = "p4_pd_" + p4_prefix + "_meter_set_" + meter
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   name = "p4_pd_" + p4_prefix + "_meter_bytecount_adjust_set_" + meter
p4_pd_status_t
${name}
(
p4_pd_sess_hdl_t sess_hdl,
p4_pd_dev_target_t dev_tgt,
int bytecount
);

//::   name = "p4_pd_" + p4_prefix + "_meter_bytecount_adjust_get_" + meter
p4_pd_status_t
${name}
(
p4_pd_sess_hdl_t sess_hdl,
p4_pd_dev_target_t dev_tgt,
int *bytecount
);

//::   name = "p4_pd_" + p4_prefix + "_meter_read_" + meter
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

/* LPF */

//:: for lpf, l_info in lpf_info.items():
//::   binding = l_info["binding"]
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     params += ["int index"]
//::   #endif
//::   params += ["p4_pd_lpf_spec_t *lpf_spec"]
//::   param_str = ",\n ".join(params)
//::   name = "p4_pd_" + p4_prefix + "_lpf_set_" + lpf
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   name = "p4_pd_" + p4_prefix + "_lpf_read_" + lpf
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

/* WRED */

//:: for wred, w_info in wred_info.items():
//::   binding = w_info["binding"]
//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "p4_pd_dev_target_t dev_tgt"]
//::   if binding[0] == "direct":
//::     params += ["p4_pd_entry_hdl_t entry_hdl"]
//::   else:
//::     params += ["int index"]
//::   #endif
//::   params += ["p4_pd_wred_spec_t *wred_spec"]
//::   param_str = ",\n ".join(params)
//::   name = "p4_pd_" + p4_prefix + "_wred_set_" + wred
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   name = "p4_pd_" + p4_prefix + "_wred_read_" + wred
p4_pd_status_t
${name}
(
 ${param_str}
);

//:: #endfor

//:: if gen_exm_test_pd == 1:
/* Activate exact match entry */

//::   for table, t_info in table_info.items():
//::     match_type = t_info["match_type"]
//::     name = p4_pd_prefix + table + "_activate_entry"
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t device_id",
//::               "p4_pd_entry_hdl_t  entry_hdl"]
//::     param_str = ",\n ".join(params)
/**
 * @brief ${name}
 * @param sess_hdl
 * @param device_id
 * @param entry_hdl
*/

p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor

/* De-Activate exact match entry */

//::   for table, t_info in table_info.items():
//::     match_type = t_info["match_type"]
//::     name = p4_pd_prefix + table + "_deactivate_entry"
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "uint8_t device_id",
//::               "p4_pd_entry_hdl_t  entry_hdl"]
//::     param_str = ",\n ".join(params)
/**
 * @brief ${name}
 * @param sess_hdl
 * @param dev_tgt
 * @param entry_hdl
*/
p4_pd_status_t
${name}
(
 ${param_str}
);

//::   #endfor
//:: #endif

typedef struct __attribute__((__packed__)) ${p4_pd_prefix}ig_snapshot_trig_spec {
//:: for name in PHV_Container_Fields[0]:
//::   width = PHV_Container_Fields[0][name]
//::   if width > 4:
  uint8_t ${name}[${width}];
//::   else:
//::     type_ = get_type(width)
  ${type_} ${name};
//::   #endif
//:: #endfor
  /* POV fields */
//:: for header_name in POV_Dict[0]:
  uint8_t ${header_name}_valid;
//:: #endfor

} ${p4_pd_prefix}ig_snapshot_trig_spec_t;


typedef struct __attribute__((__packed__)) ${p4_pd_prefix}eg_snapshot_trig_spec {
//:: for name in PHV_Container_Fields[1]:
//::   width = PHV_Container_Fields[1][name]
//::   if width > 4:
  uint8_t ${name}[${width}];
//::   else:
//::     type_ = get_type(width)
  ${type_} ${name};
//::   #endif
//:: #endfor
  /* POV fields */
//:: for header_name in POV_Dict[1]:
  uint8_t ${header_name}_valid;
//:: #endfor

} ${p4_pd_prefix}eg_snapshot_trig_spec_t;


typedef struct __attribute__((__packed__)) ${p4_pd_prefix}snapshot_trig_spec {
    union {
        ${p4_pd_prefix}ig_snapshot_trig_spec_t ig;
        ${p4_pd_prefix}eg_snapshot_trig_spec_t eg;
    } u;
}  ${p4_pd_prefix}snapshot_trig_spec_t;


typedef ${p4_pd_prefix}ig_snapshot_trig_spec_t ${p4_pd_prefix}ig_snapshot_capture_data_t;
typedef ${p4_pd_prefix}eg_snapshot_trig_spec_t ${p4_pd_prefix}eg_snapshot_capture_data_t;


typedef struct __attribute__ ((__packed__)) ${p4_pd_prefix}snapshot_capture {
     union {
         ${p4_pd_prefix}ig_snapshot_capture_data_t ig;
         ${p4_pd_prefix}eg_snapshot_capture_data_t eg;
     } u;
} ${p4_pd_prefix}snapshot_capture_t;

/* Array of snapshot captures if start and en stage are different */
typedef struct ${p4_pd_prefix}snapshot_capture_arr {
    ${p4_pd_prefix}snapshot_capture_t captures[BF_MAX_SNAPSHOT_CAPTURES];
} ${p4_pd_prefix}snapshot_capture_arr_t;


/**
 * @brief Set snapshot trigger.
 * @param hdl Snapshot handle.
 * @param trig_spec Trigger spec.
 * @param trig_mask Trigger mask.
 * @return status.
*/
p4_pd_status_t
${p4_pd_prefix}snapshot_capture_trigger_set(
              pipe_snapshot_hdl_t hdl,
              ${p4_pd_prefix}snapshot_trig_spec_t *trig_spec,
              ${p4_pd_prefix}snapshot_trig_spec_t *trig_mask);

/**
 * @brief Get snapshot capture data.
 * @param hdl Snapshot handle.
 * @param pipe Pipe.
 * @param capture Captured data
 * @param num_captures Num of captures
 * @return status.
*/
p4_pd_status_t
${p4_pd_prefix}snapshot_capture_data_get(
              pipe_snapshot_hdl_t hdl,
              bf_dev_pipe_t dev_pipe_id,
              ${p4_pd_prefix}snapshot_capture_arr_t *capture,
              bf_snapshot_capture_ctrl_info_arr_t *capture_ctrl_arr,
              int *num_captures);

/**
 * @brief Create a snapshot.
 * @param dev_tgt Device information.
 * @param start_stage_id Start stage.
 * @param end_stage_id End stage.
 * @param direction Ingress or egress
 * @param hdl Snapshot handle.
 * @return status.
*/
p4_pd_status_t
${p4_pd_prefix}snapshot_create(
            p4_pd_dev_target_t dev_tgt,
            uint8_t start_stage_id, uint8_t end_stage_id,
            bf_snapshot_dir_t direction,
            pipe_snapshot_hdl_t *hdl);

/**
 * @brief Delete snapshot.
 * @param hdl Snapshot handle.
 * @return status.
*/
p4_pd_status_t
${p4_pd_prefix}snapshot_delete(
            pipe_snapshot_hdl_t hdl);

//:: if gen_perf_test_pd == 1:
void
${p4_pd_prefix}_threads_init();

void
${p4_pd_prefix}_threads_start();

void
${p4_pd_prefix}_threads_stop();

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     match_type = t_info["match_type"]
//::     if has_match_spec == False:
//::       continue
//::     #endif
//::     params = ["p4_pd_sess_hdl_t sess_hdl",
//::               "p4_pd_dev_target_t dev_tgt"]
//::     params += ["int num_entries"]
//::     param_str = ",\n ".join(params)
//::     name = p4_pd_prefix + table + "_table_bulk_init"
p4_pd_status_t
${name}
(
 ${param_str}
);
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if action_table_hdl: continue
//::     match_type = t_info["match_type"]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     for action in t_info["actions"]:
//::       a_info = action_info[action]
//::       has_action_spec = len(a_info["param_names"]) > 0
//::       params = ["p4_pd_sess_hdl_t sess_hdl",
//::                 "p4_pd_dev_target_t dev_tgt"]
//::       if has_match_spec:
//::         params += ["const " + p4_pd_prefix + table + "_match_spec_t * const match_spec"]
//::       #endif
//::       if match_type == "ternary" or match_type == "range":
//::         params += ["int priority"]
//::       #endif
//::       if has_action_spec:
//::         params += ["const " + p4_pd_prefix + action + "_action_spec_t * const action_spec"]
//::       #endif
//::       if t_info["timeout"]:
//::         params += ["uint32_t ttl"]
//::       #endif
//::       params += get_direct_parameter_specs(table_direct_resources[table], register_info)
//::       param_str = ",\n ".join(params)
//::       name = p4_pd_prefix + table + "_table_add_with_" + action + "_bulk_setup"
p4_pd_status_t
${name}
(
 ${param_str}
);
//::     #endfor
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     name = p4_pd_prefix + table + "_churn_thread_spawn"
void
${name} ();
//::     for res_name, res_type, res_hdl in table_direct_resources[table]:
//::       if res_type == "counter":
//::         name = p4_pd_prefix + table + "_stats_ent_dump_thread_spawn"
void
${name} (p4_pd_sess_hdl_t sess_hdl,
        p4_pd_dev_target_t dev_tgt,
        p4_pd_entry_hdl_t *entry_hdls,
        int num_entry_hdls);
//::       #endif
//::     #endfor
//::   #endfor

//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     has_match_spec = len(t_info["match_fields"]) > 0
//::     if has_match_spec == False:
//::       continue
//::     #endif
//::     name = p4_pd_prefix + table + "_table_perf_test_execute"
int
${name} ();
//::   #endfor
//:: #endif

//:: if gen_md_pd:
//::   if gen_hitless_ha_test_pd:
p4_pd_status_t ${p4_pd_prefix + "enable_callbacks_for_hitless_ha"} (p4_pd_sess_hdl_t sess_hdl, uint8_t device_id);
//::   #endif
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     name = p4_pd_prefix + table + "_register_mat_update_cb"
p4_pd_status_t ${name} (p4_pd_sess_hdl_t sess_hdl,
                        bf_dev_id_t dev_id,
                        p4_pd_mat_update_cb cb,
                        void *cb_cookie);

p4_pd_status_t ${p4_pd_prefix + table + "_get_mat_table_handle"} (p4_pd_tbl_hdl_t *hdl);

//::   #endfor

//::   action_profiles = set()
//::   for table, t_info in table_info.items():
//::     table_hdl, action_table_hdl, select_hdl = table_handles[table]
//::     if not action_table_hdl: continue
//::     act_prof = t_info["action_profile"]
//::     assert(act_prof is not None)
//::     if act_prof in action_profiles: continue
//::     action_profiles.add(act_prof)
//::     name = p4_pd_prefix + act_prof + "_register_adt_update_cb"
p4_pd_status_t ${name} (p4_pd_sess_hdl_t sess_hdl,
                        bf_dev_id_t dev_id,
                        p4_pd_adt_update_cb cb,
                        void *cb_cookie);

p4_pd_status_t ${p4_pd_prefix + act_prof + "get_adt_table_handle"} (p4_pd_tbl_hdl_t *hdl);

//::     if select_hdl:
//::       name = p4_pd_prefix + act_prof + "_register_sel_update_cb"
p4_pd_status_t ${name} (p4_pd_sess_hdl_t sess_hdl,
                        bf_dev_id_t dev_id,
                        p4_pd_sel_update_cb cb,
                        void *cb_cookie);

p4_pd_status_t ${p4_pd_prefix + act_prof + "get_sel_table_handle"} (p4_pd_tbl_hdl_t *hdl);

//::     #endif
//::   #endfor

//::   name = p4_pd_prefix + "restore_virtual_dev_state"
p4_pd_status_t ${name}(p4_pd_sess_hdl_t sess_hdl,
                       bf_dev_id_t dev_id,
                       p4_pd_tbl_hdl_t tbl_hdl,
                       struct p4_pd_plcmt_info *info,
                       uint32_t *processed);
//:: #endif

//:: pvs_names = []
//:: for pvs in parser_value_set["ingress"]:
//::   pvs_names.append(pvs["pvs_name"])
//:: #endfor
//:: for pvs in parser_value_set["egress"]:
//::   if pvs["pvs_name"] not in pvs_names:
//::     pvs_names.append(pvs["pvs_name"])
//::   #endif
//:: #endfor
//:: for pvs_name in pvs_names:

//::   name = p4_pd_prefix + pvs_name + "_set_property"
/**
 * @brief This API can be used to set property of parser value set. This can be
 * 	  use to set the gress scope of the pvs instance as well as the pipe
 * 	  and the parser scope of the pvs instance within the gresses 
 *
 *        Once property is set, it can only be modified if none of the PVS 
 * 	  set elements are in use. User has to free up all pvs elements by 
 * 	  using _entry_delete API on each of the entry handles previously 
 *	  obtained at the time of _entry_add API
 *
 * @param sess_hdl : Session handle
 * @param dev_id : device id to which pvs scope is localized to.
 * @param property : PVS property; which can be gress or pipe or parser scope
 * @param value : PVS property value
 * @param args : PVS property args; which are used to specify the gress instance
 *               in which the pipe or parser scope property is to be set. This 
 *               parameter is unusued when setting gress scope property
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_set_property"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 int                        dev_id,
 p4_pd_pvs_prop_type_t      property,
 p4_pd_pvs_prop_value_t     value,
 p4_pd_pvs_prop_args_t      value_args
);

/**
 * @brief This API can be used to get property of parser value set. This can be
 * 	  use to get the gress scope of the pvs instance as well as the pipe
 * 	  and the parser scope of the pvs instance within the gresses 
 *
 * @param sess_hdl : Session handle
 * @param dev_id : device id to which pvs scope is localized to.
 * @param property : PVS property; which can be gress or pipe or parser scope
 * @param value : PVS property value
 * @param args : PVS property args; which are used to specify the gress instance
 *               in which the pipe or parser scope property is to be fetched from. 
 *		 This parameter is unusued when getting gress scope property
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_get_property"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 int                        dev_id,
 p4_pd_pvs_prop_type_t      property,
 p4_pd_pvs_prop_value_t     *value,
 p4_pd_pvs_prop_args_t      value_args
);

/**
 * @brief This API can be used to add an entry in a parser value set. An element within
 *        the array of parser value set can be set to constant value which
 *        packet parser can use to branch on in parse graph.
 *        The API is intended to be used in conjunction with parser_value_set
 *        construct in p4 program so that parser select value can be changed
 *        at runtime.
 * @param sess_hdl : Session handle
 * @param prsr_tgt : Identifies target parser. Target parser is built using API
 *                   ${name}
 * @param parser_value : The value that packet parser should use to branch in
 *                       select condition
 * @param parser_value_mask : Mask applied to parser_value
 * @param pvs_entry_handle : Parser value set handle allocated for the entry that got added.
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_entry_add"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 p4_pd_dev_parser_target_t  prsr_tgt,
 uint32_t                   parser_value,
 uint32_t                   parser_value_mask,
 p4_pd_pvs_hdl_t            *pvs_entry_handle
);

/**
 * @brief This API can be used to modify an entry in a parser value set. An element within
 *        the array of parser value set can be updated to a new constant value
 *        which packet parser can use to branch on in parse graph. Old parser
 *        value will be overwritten.
 *        The API is intended to be used in conjunction with parser_value_set
 *        construct in p4 program so that parser select value can be changed
 *        at runtime.
 * @param sess_hdl : Session handle
 * @param prsr_tgt : Identifies target parser. Target parser is built using API
 *                   ${name}
 * @param pvs_entry_handle : Parser value set entry handle allocated when new entry was added.
 * @param parser_value : The value that packet parser should use to branch in
 *                       select condition
 * @param parser_value_mask : Mask applied to parser_value
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_entry_modify"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 int                        dev_id,
 p4_pd_pvs_hdl_t            pvs_entry_handle,
 uint32_t                   parser_value,
 uint32_t                   parser_value_mask
);

/* @brief This API can be used to delete a parser value set element.
 *        The API is intended to be used in conjunction with parser_value_set
 *        construct in p4 program so that parser select value can be
 *        removed at runtime.
 * @param sess_hdl : Session handle
 * @param prsr_tgt : Identifies target parser. Target parser is built using API
 *                   ${name}
 * @param pvs_entry_handle : Parser value set entry handle allocated when new entry was added.
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_entry_delete"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 int                        dev_id,
 p4_pd_pvs_hdl_t            pvs_entry_handle
);

/* @brief This API can be used to get the pvs spec corresponding to the entry
 *  	  installed
 * @param sess_hdl : Session Handle
 * @param dev_id   : Device id
 * @param p4_pd_pvs_hdl_t : Parser value set handle of the entry to be read
 * @param parser_value : The value that parser uses to branch in select
			 condition
 * @param parser_value_mask : Mask applied to parser_value
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_entry_get"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t    sess_hdl,
 int                 dev_id,
 p4_pd_pvs_hdl_t     pvs_entry_handle,
 uint32_t            *parser_value,
 uint32_t            *parser_value_mask
);

/* @brief This API can be used to get the entry handle associated with an
 * 	  installed parser_value_set entry
 *
 * @param sess_hdl : Session Handle
 * @param prsr_tgt : Identifies target parser. Target parser is built using API
 *                   ${name}
 * @param parser_value : The value that packet parser should use to branch in
 *                       select condition
 * @param parser_value_mask : Mask applied to parser_value
 * @param pvs_entry_handle : Parser value set entry handle of an entry if it 
 *  	  		     exists.
 * @return status.
 */
//::   name = p4_pd_prefix + pvs_name + "_entry_handle_get"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 p4_pd_dev_parser_target_t  prsr_tgt,
 uint32_t                   parser_value,
 uint32_t                   parser_value_mask,
 p4_pd_pvs_hdl_t            *pvs_entry_handle
);

//::   name = p4_pd_prefix + pvs_name + "_entry_get_first_entry_handle"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 p4_pd_dev_parser_target_t  prsr_tgt,
 p4_pd_pvs_hdl_t            *pvs_entry_handle
);

//::   name = p4_pd_prefix + pvs_name + "_entry_get_next_entry_handles"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 p4_pd_dev_parser_target_t  prsr_tgt,
 p4_pd_pvs_hdl_t            pvs_entry_handle,
 int                        n,
 p4_pd_pvs_hdl_t            *next_entry_handles
);

//::   name = p4_pd_prefix + pvs_name + "_entry_get_count"
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t           sess_hdl,
 p4_pd_dev_parser_target_t  prsr_tgt,
 bool                       read_from_hw,
 uint32_t                   *count
);

//:: #endfor


//:: field_lists = set()
//:: for hash_calc_name, dyn_hash_calc_info in hash_calc.items():
//::   for fl_name, fl_info in dyn_hash_calc_info['field_lists'].items():
//::     if fl_name in field_lists:
//::       continue
//::     #endif
//::     field_lists.add(fl_name)
typedef enum ${p4_pd_prefix}input_fields_${fl_name}{
//::     enum_pos = 0
//::     for f_info in fl_info['fields']:
//::       enum_name_upper = "P4_PD_INPUT_FIELD_" + fl_name.upper() + "_" + f_info['name'].upper()
  ${enum_name_upper} = ${enum_pos},
//::       enum_pos += 1
//::     #endfor
} ${p4_pd_prefix}input_fields_${fl_name}_t;
//::   #endfor
//:: #endfor

typedef enum ${p4_pd_prefix}input_field_attr_type {
  P4_PD_INPUT_FIELD_ATTR_TYPE_MASK = 0,
  P4_PD_INPUT_FIELD_ATTR_TYPE_VALUE,
} ${p4_pd_prefix}input_field_attr_type_t;

typedef enum ${p4_pd_prefix}input_field_attr_value_mask {
  P4_PD_INPUT_FIELD_EXCLUDED = 0,
  P4_PD_INPUT_FIELD_INCLUDED
} ${p4_pd_prefix}input_field_attr_value_mask_t;

//:: for hash_calc_name, dyn_hash_calc_info in hash_calc.items():
typedef enum ${p4_pd_prefix}${hash_calc_name}_input{
//::   for fl_name, fl_info in dyn_hash_calc_info['field_lists'].items():
//::     enum_name_upper = p4_pd_prefix.upper() + hash_calc_name.upper() + "_INPUT_" + fl_name.upper()
  ${enum_name_upper},
//::   #endfor
} ${p4_pd_prefix}${hash_calc_name}_input_t;

typedef struct ${p4_pd_prefix}${hash_calc_name}_input_field_attribute{
  union {
//::   for fl_name, fl_info in dyn_hash_calc_info['field_lists'].items():
    ${p4_pd_prefix}input_fields_${fl_name}_t ${fl_name};
//::   #endfor
    uint32_t id;
  } input_field;
  ${p4_pd_prefix}input_field_attr_type_t type;
  union {
    ${p4_pd_prefix}input_field_attr_value_mask_t mask;
    uint32_t val;
    uint8_t *stream;
  } value;
} ${p4_pd_prefix}${hash_calc_name}_input_field_attribute_t;

typedef enum ${p4_pd_prefix}${hash_calc_name}_algo {
//::   for alg_name, handle in dyn_hash_calc_info['algs']:
//::     enum_name_upper = p4_pd_prefix.upper() + hash_calc_name.upper() + "_ALGORITHM_" + alg_name.upper()
  ${enum_name_upper},
//::   #endfor
} ${p4_pd_prefix}${hash_calc_name}_algo_t;

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_t input"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_input_set"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_t *input"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_input_get"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_algo_t algo"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_algorithm_set"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_algo_t *algo"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_algorithm_get"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "uint64_t seed"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_seed_set"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id",
//::             "uint64_t *seed"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_seed_get"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_t input"]
//::   params += ["uint32_t attr_count"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_field_attribute_t *array_of_attrs"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_input_field_attribute_set"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_t input"]
//::   params += ["uint32_t *attr_count"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_input_field_attribute_count_get"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_t input"]
//::   params += ["uint32_t max_attr_count"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_field_attribute_t *array_of_attrs"]
//::   params += ["uint32_t *num_attr_filled"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_input_field_attribute_get"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);

//::   params = ["p4_pd_sess_hdl_t sess_hdl",
//::             "bf_dev_id_t dev_id"]
//::   params += ["uint32_t attr_count"]
//::   params += [p4_pd_prefix + hash_calc_name + "_input_field_attribute_t *array_of_attrs"]
//::   params += ["uint32_t *attr_sizes"]
//::   params += ["uint8_t *hash"]
//::   params += ["uint32_t hash_len"]
//::   param_str = ",\n ".join(params)
//::   fn_name = p4_pd_prefix + "hash_calc_" + hash_calc_name + "_calculate_hash_value"
p4_pd_status_t
${fn_name}
(
 ${param_str}
);
//:: #endfor

//:: if gen_hitless_ha_test_pd:
//::   for table, t_info in table_info.items():
//::     name = p4_pd_prefix + table + "_get_ha_reconciliation_report"
//::     params = ["p4_pd_sess_hdl_t sess_hdl", "p4_pd_dev_target_t dev_tgt"]
//::     params += [p4_pd_prefix + "ha_reconc_report_t *ha_report"]
//::     param_str = ",\n ".join(params)
p4_pd_status_t
${name}
(
 ${param_str}
);
//::   #endfor
//:: #endif
#endif
