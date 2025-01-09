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


/*
   action act1(mac_addr_t srcmac, ipv4_addr_t ipaddr) {
        ethernet.srcMacAddr = srcmac;
        ipv4.srcAddr = ipaddr;
   }

   action act2(switch_port_t egress_port) {
        ig_intr_md_for_tm.ucast_egress_port = egress_port;
    }

   table example_tbl {
       key = {
           ethernet.dstMacAddr : exact;
           ipv4.dstAddr : exact;
       }
       actions {
           act1;
           act2;
       }

       size = 1024;
   }
*/

/* Example to add a match entry with direct action data */
void add_entry_direct_action_example() {
  bf_status_t status = BF_SUCCESS;
  bf_rt_table_key_t *table_key = NULL;
  bf_rt_table_data_t *table_data = NULL;

  /* First step is to create the bf_rt --> For not, soon this will be implicitly
   * done using device add
   */

  bf_rt_info_hdl *bf_rt_hdl = NULL;
  /* This returns a handle to the bf_rt_info */
  status = bf_rt_create(path_to_bf_rt_info, path_to_ctx_json, &bf_rt_hdl);

  /* Next step is to get the table handle from id */
  bf_rt_table_hdl *bf_rt_table = NULL;
  status =
      bf_rt_table_from_name_get(bf_rt_info_hdl, "example_tbl", &bf_rt_table);

  /* Next get a list of key fields */
  bf_rt_table_key_field_hdl **key_field_list = NULL;
  uint32_t num_key_fields = 0;
  status = bf_rt_table_get_key_field_list(
      bf_rt_table_hdl, &num_key_fields, &key_field_list);

  /* Next get a list of data fields */
  bf_rt_table_data_field_hdl **data_field_list = NULL;
  status = bf_rt_table_get_data_field_list(
      bf_rt_table_hdl, &num_data_fields, &data_field_list);

  /* Next, allocate key for the table */
  bf_rt_table_key_hdl *key_hdl;
  status = bf_rt_table_key_allocate(bf_rt_table_hdl, &key_hdl);

  /* Next, get action id from name */
  bf_rt_id_t action_id = 0;
  char *action_name = "act1";
  status = bf_rt_action_name_to_id(bf_rt_table_hdl, action_name, &action_id);

  /* Next, allocate data for the table */
  bf_rt_table_data_hdl *data_hdl;
  status = bf_rt_table_data_allocate(bf_rt_table, action_id, &data_hdl);

  /* All the above steps thus far can be done one-time per table and
   * "initialize" functions can be used to clean those objects for re-use
   */

  /* Iterate over all key fields and set them */
  unsigned i = 0;
  for (i = 0; i < num_key_fields; i++) {
    bf_rt_table_key_field_hdl *field = key_field_list[i];
    bf_rt_id_t key_field_id = 0;
    status = bf_rt_key_field_get_field_id(key_hdl, field, &field_id);
    switch (key_field_id) {
      case EXACT_KEY:
        /* Exact key, use the set_value function */
        status = bf_rt_key_set_field(key_hdl, key_field_id, value);
        break;
      case TERNARY_KEY:
        /* Ternary key, set key and mask */
        status = bf_rt_key_field_set_value_and_mask(field, value, mask);
        break;
      case LPM_KEY:
        /* LPM key, set value and prefix length */
        status = bf_rt_key_field_set_value_lpm(field, value, prefix_len);
        break;
      case RANGE_KEY:
        /* Range key, set range low and range hi */
        status = bf_rt_key_field_set_value_range(field, range_lo, range_hi);
        break;
      default:
        break;
    }
  }

  /* Next, iterate over all data fields and set them */
  for (i = 0; i < num_data_fields; i++) {
    bf_rt_table_data_field_hdl *data_field = data_field_list[i];
    bf_rt_id_t data_field_id = 0;
    status = bf_rt_data_field_get_field_id(field, &key_field_id);
  }

  /* Get table id, and match field ids from name */
  bf_rt_tbl_id_t tbl_id = bf_rt_table_id_get(p4info, "example_tbl");

  /* Get field ids within the match spec */
  bf_rt_id_t macAddr_field =
      bf_rt_table_key_field_id_get(p4info, tbl_id, "ethernet.dstMacAddr");
  bf_rt_id_t ipv4dstAddr_field =
      bf_rt_table_key_field_id_get(p4info, tbl_id, "ipv4.dstAddr");

  /* Get action id and action data fields from name */
  /* The action name is scoped within the match table */
  bf_rt_id_t act1_id = bf_rt_table_data_action_id_get(p4info, tbl_id, "act1");

  bf_rt_id_t act_macAddr_field_id = bf_rt_table_data_field_id_get_with_action(
      p4info, tbl_id, act1_id, "ethernet.dstMacAddr");
  bf_rt_id_t act_ipAddr_field_id = bf_rt_table_data_field_id_get_with_action(
      p4info, tbl_id, act1_id, "ipv4.dstAddr");

  /* Allocate match table key */
  status = bf_rt_table_key_allocate(p4info, tbl_id, &table_key);

  uint32_t ipAddr = 0xa010203;
  uint64_t macAddr = 0xaabbccddeeff;

  /* Get an iterator over the fields of the table key */
  bf_rt_table_key_field_t *key_fields = NULL;
  uint32_t num_fields;
  status = bf_rt_table_key_fields_allocate(table_key, key_fields, &num_fields);

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (field_info_arr[i].field_id) {
      case macAddr_field:
        field_info_arr[i].value.u64 = macAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u64_exact_set(&key_fields[i], macAddr);
        break;
      case ipv4dstAddr_field:
        field_info_arr[i].value.u32 = ipAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&key_fields[i], ipAddr);
        break;
      default:
        printf("Unknown field id\n");
        break;
    }
  }
  status = bf_rt_table_key_set(table_key, num_fields, key_fields);

  /* Action data spec */
  status = bf_rt_table_data_allocate_with_action(p4info, act1_id, &table_data);

  /* Set action MacAddress */
  uint64_t act_macAddr = 0x10203040506;
  /* Set action ipAddress */
  uint32_t act_ipAddr = 0x14010203;

  /* Get an iterator over the fields of the table data */
  bf_rt_table_data_fields_t *data_fields = NULL;
  status =
      bf_rt_table_data_fields_allocate(table_data, &data_fields, &num_fields);

  for (i = 0; i < num_fields; i++) {
    switch (data_fields[i].field_id) {
      case act_macAddr_field_id:
        bf_rt_table_data_u64_set(&data_fields[i], act_macAddr);
        break;
      case act_ipAddr_field_id:
        bf_rt_table_data_u32_set(&data_fields[i], act_ipAddr);
        break;
      default:
        printf("Unknown field id\n");
        break;
    }
  }

  status = bf_rt_table_data_set(table_data, data_fields, num_fields);

  /* Add the entry */
  status =
      bf_rt_table_entry_add(sess_hdl, dev_tgt, tbl_id, table_key, table_data);

  /* Cleanup */
  /* 1. Destroy allocated table key */
  status = bf_rt_table_key_destroy(table_key);

  /* 2. Destroy allocated table data */
  status = bf_rt_table_data_destroy(table_data);

  /* 3. Destroy table key fields */
  status = bf_rt_table_key_fields_destroy(key_fields);

  /* 4. Destroy table data fields */
  status = bf_rt_table_data_fields_destroy(data_fields);

  return;
}

/*
   action act1(mac_addr_t srcmac, ipv4_addr_t ipaddr) {
        ethernet.srcMacAddr = srcmac;
        ipv4.srcAddr = ipaddr;
   }

   action act2(switch_port_t egress_port) {
        ig_intr_md_for_tm.ucast_egress_port = egress_port;
    }

   table example_tbl {
       key = {
           ethernet.dstMacAddr : exact;
           ipv4.dstAddr : exact;
       }
       actions {
           act1;
           act2;
       }
       implementation = example_action_profile;
       size = 1024;
   }
*/

void add_entry_indirect_action_example() {
  bf_status_t status = BF_SUCCESS;
  bf_rt_table_key_t *table_key = NULL;
  bf_rt_table_data_t *table_data = NULL;

  /* Get action profile id from name */
  bf_rt_tbl_id_t act_prof_id =
      bf_rt_table_id_get(p4info, "example_action_profile");

  /* Get action id and action data fields from name */
  /* The action name is scoped within the match table */
  bf_rt_id_t act1_id = bf_rt_table_data_action_id_get(p4info, tbl_id, "act1");

  bf_rt_id_t act_macAddr_field_id = bf_rt_table_data_field_id_with_action_get(
      p4info, tbl_id, act1_id, "ethernet.dstMacAddr");
  bf_rt_id_t act_ipAddr_field_id = bf_rt_table_data_field_id_with_action_get(
      p4info, tbl_id, act1_id, "ipv4.dstAddr");
  /* Set action MacAddress */
  uint64_t act_macAddr = 0x10203040506;
  /* Set action ipAddress */
  uint32_t act_ipAddr = 0x14010203;

  /* Allocate action profile key */
  status = bf_rt_table_key_allocate(p4info, act_prof_id, &table_key);

  bf_rt_table_key_field_t *data_fields = NULL;
  uint32_t num_fields;
  bf_rt_id_t action_member_id =
      bf_rt_table_key_field_id_get(p4info, tbl_id, "action_member_id");

  /* Allocate table key fields */
  status = bf_rt_table_key_fields_allocate(table_key, data_fields, &num_fields);

  uint32_t act_prof_mbr_id = 1;

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (field_info_arr[i].field_id) {
      case action_member_id:
        field_info_arr[i].value.u32 = act_prof_mbr_id;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&data_fields[i], act_prof_mbr_id);
        break;
      default:
        printf("Unknown field id\n");
        break;
    }
  }

  status = bf_rt_table_key_set(table_key, num_fields, data_fields);

  /* Allocate action profile table data */
  bf_rt_table_data_t *act_prof_data = NULL;
  status = bf_rt_table_data_allocate_with_action(
      act_prof_id, act1_id, &act_prof_data);

  bf_rt_table_key_field_t *key_fields = NULL;
  uint32_t num_fields;
  status =
      bf_rt_table_data_fields_allocate(act_prof_data, key_fields, &num_fields);

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (field_info_arr[i].field_id) {
      case act_macAddr_field_id:
        field_info_arr[i].value.u64 = act_macAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u64_exact_set(&key_fields[i], act_macAddr);
        break;
      case act_ipAddr_field_id:
        field_info_arr[i].value.u32 = act_ipAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&key_fields[i], ipAddr);
        break;
      default:
        printf("Unknown field id\n");
        break;
    }
  }

  /* Add the action profile member to the action profile table */
  status = bf_rt_table_entry_add(
      sess_hdl, dev_tgt, act_prof_id, table_key, act_prof_data);

  /* Now add the Match entry */
  /* Allocate match table key */
  status = bf_rt_table_key_allocate(p4info, tbl_id, &table_key);

  uint32_t ipAddr = 0xa010203;
  uint64_t macAddr = 0xaabbccddeeff;

  /* Get an iterator over the fields of the table key */
  bf_rt_table_key_field_t *key_fields = NULL;
  uint32_t num_fields;
  status = bf_rt_table_key_fields_allocate(table_key, key_fields, &num_fields);

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (field_info_arr[i].field_id) {
      case macAddr_field:
        field_info_arr[i].value.u64 = macAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u64_exact_set(&key_fields[i], macAddr);
        break;
      case ipv4dstAddr_field:
        field_info_arr[i].value.u32 = ipAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&key_fields[i], ipAddr);
        break;
      default:
        break;
    }
  }

  status = bf_rt_table_key_set(table_key, num_fields, key_fields);

  /* Allocate match table data */
  status = bf_rt_table_data_allocate(p4info, &table_data);

  bf_rt_id_t key_action_member_id =
      bf_rt_table_data_field_id_get(p4info, tbl_id, "action_member_id");

  /* Get an iterator over the fields of the table data */
  bf_rt_table_data_fields_t *data_arr = NULL;
  status =
      bf_rt_table_data_fields_allocate(p4info, tbl_id, &data_arr, &num_fields);

  for (i = 0; i < num_fields; i++) {
    switch (data_arr[i].field_id) {
      case key_action_member_id:
        bf_rt_table_data_field_u32_set(&data_arr[i], act_prof_mbr_id);
        break;
      default:
        break;
    }
  }

  status = bf_rt_table_data_set(tbl_data, data_arr, num_fields);

  /* Add match entry */
  status = bf_rt_table_entry_add(sess_hdl, dev_tgt, tbl_id, table_key, &data);

  /* Cleanup */
  /* 1. Destroy allocated table key */
  status = bf_rt_table_key_destroy(table_key);

  /* 2. Destroy allocated table data */
  status = bf_rt_table_data_destroy(table_data);

  /* 3. Destroy table key fields */
  status = bf_rt_table_key_fields_destroy(key_fields);

  /* 4. Destroy table data fields */
  status = bf_rt_table_data_fields_destroy(data_fields);

  return;
}

/*
   ActionSelector(HashAlgorithm_t.IDENTITY, SELECTOR_TABLE_SIZE,
   SELECTOR_HASH_WIDTH) example_action_selector;

   action act1(mac_addr_t srcmac, ipv4_addr_t ipaddr) {
        ethernet.srcMacAddr = srcmac;
        ipv4.srcAddr = ipaddr;
   }

   action act2(switch_port_t egress_port) {
        ig_intr_md_for_tm.ucast_egress_port = egress_port;
    }

   table example_tbl {
       key = {
           ethernet.dstMacAddr : exact;
           ipv4.dstAddr : exact;
       }
       actions {
           act1;
           act2;
       }
       implementation = example_action_selector;
       size = 1024;
   }
*/

/* Helper function to create an action profile entry with a given action */
void add_action_profile_member_with_act1(uint64_t macAddr,
                                         uint32_t ipAddr,
                                         uint32_t mbr_id,
                                         bf_rt_id_t act_id,
                                         bf_rt_id_t action_profile_id) {
  bf_rt_table_key_t *table_key = NULL;
  bf_rt_table_key_field_t *key_fields = NULL;

  bf_rt_id_t act_macAddr_field_id = bf_rt_table_data_field_id_with_action_get(
      p4info, tbl_id, act1_id, "ethernet.dstMacAddr");
  bf_rt_id_t act_ipAddr_field_id = bf_rt_table_data_field_id_with_action_get(
      p4info, tbl_id, act1_id, "ipv4.dstAddr");

  /* Allocate action profile key */
  status = bf_rt_table_key_allocate(p4info, act_prof_id, &table_key);

  uint32_t num_fields;
  bf_rt_id_t action_member_id =
      bf_rt_table_key_field_id_get(p4info, tbl_id, "action_member_id");

  /* Allocate table key fields */
  status = bf_rt_table_key_fields_allocate(table_key, key_fields, &num_fields);

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (field_info_arr[i].field_id) {
      case action_member_id:
        field_info_arr[i].value.u32 = mbr_id;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&data_fields[i], mbr_id);
        break;
      default:
        printf("Unknown field id\n");
        break;
    }
  }

  status = bf_rt_table_key_set(table_key, num_fields, data_fields);

  /* Allocate action profile table data */
  bf_rt_table_data_t *act_prof_data = NULL;
  status = bf_rt_table_data_allocate_with_action(
      act_prof_id, act_id, &act_prof_data);

  bf_rt_table_data_field_t *data_fields = NULL;
  status =
      bf_rt_table_data_fields_allocate(act_prof_data, data_fields, &num_fields);

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (field_info_arr[i].field_id) {
      case act_macAddr_field_id:
        field_info_arr[i].value.u64 = macAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u64_exact_set(&key_fields[i], macAddr);
        break;
      case act_ipAddr_field_id:
        field_info_arr[i].value.u32 = ipAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&key_fields[i], ipAddr);
        break;
      default:
        printf("Unknown field id\n");
        break;
    }
  }

  /* Add the action profile member to the action profile table */
  status = bf_rt_table_entry_add(
      sess_hdl, dev_tgt, action_profile_id, table_key, act_prof_data);

  return;
}

/* Helper function to create a group with members for an action profile table */
void add_group_to_action_profile(uint32_t *mbr_arr,
                                 bool *mbr_status,
                                 uint32_t num_members,
                                 uint32_t max_group_size,
                                 uint32_t grp_id,
                                 bf_rt_id_t grp_tbl_id) {
  bf_rt_table_key_t *table_key = NULL;
  bf_rt_table_data_t *table_data = NULL;
  bf_rt_table_key_field_t *key_fields = NULL;
  bf_rt_table_data_field_t *data_fields = NULL;

  /* Allocate group table key */
  status = bf_rt_table_key_allocate(p4info, grp_tbl_id, &table_key);

  uint32_t num_fields;
  bf_rt_id_t group_id_key =
      bf_rt_table_key_field_id_get(p4info, grp_tbl_id, "group_id");

  /* Allocate table key fields */
  status = bf_rt_table_key_fields_allocate(table_key, key_fields, &num_fields);

  /* Set key field using the field-id */
  status = bf_rt_table_key_field_u32_set(key_fields, group_id_key, grp_id);

  /* Set the key from the fields */
  status = bf_rt_table_key_set(table_key, num_fields, key_fields);

  /* Allocate table data fields */
  status = bf_rt_table_data_allocate(grp_tbl_id, &table_data);

  status =
      bf_rt_table_data_fields_allocate(table_data, data_fields, &num_fields);

  /* Get field-ids for various fields that make up the data */
  bf_rt_id_t max_group_size_field_id =
      bf_rt_table_data_field_id_get(p4info, grp_tbl_id, "max_group_size");
  bf_rt_id_t member_array_field_id =
      bf_rt_table_data_field_id_get(p4info, grp_tbl_id, "member_array");
  bf_rt_id_t member_status_field_id =
      bf_rt_table_data_field_id_get(p4info, grp_tbl_id, "member_status_array");

  /* Set the fields */
  for (i = 0; i < num_fields; i++) {
    switch (data_fields[i].field_id) {
      case max_group_size_field_id:
        data_fields[i].value.u32 = max_group_size;
        break;
      case member_array_field_id:
        data_fields[i].value.u32_arr = mbr_arr;
        break;
      case member_status_field_id:
        data_fields[i].value.bool_arr = mbr_status;
        break;
      default:
        break;
    }
  }

  /* Set the data from the fields */
  status = bf_rt_table_data_set(table_data, num_fields, data_fields);

  /* Add the entry */
  status = bf_rt_table_entry_add(
      sess_hdl, dev_tgt, grp_tbl_id, table_key, table_data);

  return;
}

void add_entry_indirect_action_with_selector_example() {
  bf_status_t status = BF_SUCCESS;
  bf_rt_table_key_t *table_key = NULL;
  bf_rt_table_data_t *table_data = NULL;

  /* Get action profile id from name */
  bf_rt_tbl_id_t act_prof_id =
      bf_rt_table_id_get(p4info, "example_action_profile");

  /* Get action id and action data fields from name */
  /* The action name is scoped within the match table */
  bf_rt_id_t act1_id = bf_rt_table_data_action_id_get(p4info, tbl_id, "act1");

  /* Set action MacAddress */
  uint64_t act_macAddr = 0x10203040506;
  /* Set action ipAddress */
  uint32_t act_ipAddr = 0x14010203;

  add_action_profile_member_with_act1(
      act_macAddr, act_ipAddr, 1, act1_id, act_prof_id);

  /* Add another member with the same action */

  /* Set action MacAddress */
  act_macAddr = 0x112233445566;
  /* Set action ipAddress */
  act_ipAddr = 0x14140203;

  add_action_profile_member_with_act1(
      act_macAddr, act_ipAddr, 2, act1_id, act_prof_id);

  /* Now, create a group in the action profile table consisting of the above
   * members
   */

  /* 1. Get the group table from the action profile table */
  bf_rt_id_t act_prof_grp_tbl_id =
      bf_rt_group_table_id_get(p4info, act_prof_id);

  /* 2. Allocate an array of members that is desired to be in the group */
  uint32_t mbr_arr[2] = {1, 2};

  /* 3. Invoke a helper function to add the group */
  add_group_to_action_profile(mbr_arr,
                              2 /* Num members */,
                              10,
                              /* Max group size */,
                              1,
                              /* Group id */,
                              act_prof_grp_tbl_id);

  /* Now add the Match entry */
  /* Allocate match table key */
  status = bf_rt_table_key_allocate(p4info, tbl_id, &table_key);

  uint32_t ipAddr = 0xa010203;
  uint64_t macAddr = 0xaabbccddeeff;

  /* Get an iterator over the fields of the table key */
  bf_rt_table_key_field_t *key_fields = NULL;
  uint32_t num_fields;
  status = bf_rt_table_key_fields_allocate(table_key, key_fields, &num_fields);

  unsigned i = 0;
  for (i = 0; i < num_fields; i++) {
    switch (key_fields[i].field_id) {
      case macAddr_field:
        key_fields[i].value.u64 = macAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u64_exact_set(&key_fields[i], macAddr);
        break;
      case ipv4dstAddr_field:
        key_fields[i].value.u32 = ipAddr;
        /* For more type-checking use a inline function */
        bf_rt_table_key_field_u32_exact_set(&key_fields[i], ipAddr);
        break;
      default:
        break;
    }
  }

  status = bf_rt_table_key_set(table_key, num_fields, key_fields);

  /* Allocate match table data */
  status = bf_rt_table_data_allocate(p4info, &table_data);

  bf_rt_id_t key_action_member_id =
      bf_rt_table_data_field_id_get(p4info, tbl_id, "action_member_id");

  /* Get an iterator over the fields of the table data */
  bf_rt_table_data_fields_t *data_arr = NULL;
  status =
      bf_rt_table_data_fields_allocate(p4info, tbl_id, &data_arr, &num_fields);

  for (i = 0; i < num_fields; i++) {
    switch (data_arr[i].field_id) {
      case key_action_member_id:
        bf_rt_table_data_field_u32_set(&data_arr[i], act_prof_mbr_id);
        break;
      default:
        break;
    }
  }

  status = bf_rt_table_data_set(tbl_data, data_arr, num_fields);

  /* Add match entry */
  status = bf_rt_table_entry_add(sess_hdl, dev_tgt, tbl_id, table_key, &data);

  /* Cleanup */
  /* 1. Destroy allocated table key */
  status = bf_rt_table_key_destroy(table_key);

  /* 2. Destroy allocated table data */
  status = bf_rt_table_data_destroy(table_data);

  /* 3. Destroy table key fields */
  status = bf_rt_table_key_fields_destroy(key_fields);

  /* 4. Destroy table data fields */
  status = bf_rt_table_data_fields_destroy(data_fields);

  return;
}
