# Access Control List (ACL)
This document describes the ACL feature for SONiC and integration with Barefoot stack.


## Overview

Access Control List is a user-ordered set of rules to configure the forwarding behavior in a switch. Each rule defines a match on a packet and defines the actions that will be performed on the packet.

In SONiC, ACLs are supported at different stages in the forwarding pipeline like Ingress, Egress etc. Each ACL is typically associated with one or more ports or port-channels. This association of ACL to a port or port-channel is called as the bind point for the ACL.

ACLs can be added either explicitly by the user through configuration commands, or created internally by the SONiC features. In both cases SONiC uses the SAI interface to program the ACLs into the SDK/Hardware. The flow of ACL configuration through different layers are explained subsequently.


## Configuration

Typically in SONiC, an ACL is configured using following steps:

1. Creating ACL table and binding it to a list of ports
2. Creating ACL rules in each table that specify match/action


### Creating ACL table
This takes the following parameters:

Key: ACL table name

Type: This can be L3, L3V6, MIRROR, etc. In addition, a custom table type can also be configured.

Stage: Ingress, Egress, Pre-Ingress

Ports: These are the ports the ACL table will be bound to. One ACL table can be bound to multiple ports.

Policy_desc: This is a short description about the ACL table.

For example, an ACL table with key "L3_ACL" as name is configured
```
    "ACL_TABLE": {
        "L3_ACL": {
            "stage": "INGRESS",
            "type": "L3",
            "policy_desc": "L3 ACL table",
            "ports": [
                "Ethernet0",
                "Ethernet4"
            ]
        }
    }
```
The match/action criteria for the Table is predefined by SONiC in the implementation files.

SONiC also supports creating ACL table with custom criteria. This is done using a custom ACL table type. For creating the custom ACL table type, the configuration is as follows:

Key: ACL_table_type

Matches: Defines the fields that the ACL table can be matched on

Actions: Defines the set of actions the table can perform

Bind_points: Defines where the table can be associated

An example table type called "CUSTOM" is configured below:
```
    "ACL_TABLE_TYPE": {
        "CUSTOM": {
            "matches": [
                "IN_PORTS",
                "SRC_IP"
            ],
            "actions": [
                "PACKET_ACTION",
                "MIRROR_INGRESS_ACTION"
            ],
            "bind_points": [
                "PORT",
                "PORTCHANNEL"
            ]
        }
    },
```

Once the custom ACL table type is created, the ACL table can use it as a value for the "type" field.

### Creating ACL rule
The ACL rule or the ACL entry defines the match values and actions. Each ACL rule belongs to a certain ACL table. The ACL rule uses the following fields for configuration.

Key: Table_name|Rule_name

Priority: This provides the ordering. Higher number means higher priority and the rule with highest priority is chosen when multiple rules match.

Packet_Action: DROP, FORWARD, REDIRECT, DO_NOT_NAT

IP_Type: IPV4ANY, IPV6ANY, ARP, etc.

For example, 2 ACL rules named "RULE0" and "RULE1" are added to the ACL table named "L3_ACL".

```
    "ACL_RULE": {
        "L3_ACL|RULE0": {
            "PRIORITY": "10",
            "PACKET_ACTION": "DROP",
            "IP_TYPE": "IPv4ANY",
            "SRC_IP": "1.1.1.1/32"
        },
        "L3_ACL|RULE1": {
            "PRIORITY": "20",
            "PACKET_ACTION": "DROP",
            "IP_TYPE": "IPv4ANY",
            "DST_IP": "2.2.2.2/32"
        }
    }

```


Once the ACL table and ACL rules are configured, the following show commands can be used to verify the configuration:

```
show acl table
show acl rule
```
For example:


```
root@sonic:/home/admin# show acl table
Name    Type    Binding    Description    Stage
------  ------  ---------  -------------  -------
L3_ACL  L3      Ethernet0  L3 ACL table   ingress
                Ethernet4
root@sonic:/home/admin# show acl rule
Table    Rule    Priority    Action    Match
-------  ------  ----------  --------  ------------------
L3_ACL   RULE1   20          DROP      DST_IP: 2.2.2.2/32
                                       IP_TYPE: IPv4ANY
L3_ACL   RULE0   10          DROP      IP_TYPE: IPv4ANY
                                       SRC_IP: 1.1.1.1/32
root@sonic:/home/admin#
```


## Design/Implementation
### SONiC
In order to support multiple ports configured for same ACL table, SONiC uses the notion of ACL table group and ACL table group members to provide indirection.

An ACL table group basically consists of one or more ACL table group members. Each ACL group member then point to an ACL table. The ACL table group is then attached to a bindpoint like a port.

When an ACL table is bound to a port at configuration, internally SONiC creates the ACL group, adds ACL group member pointing to the ACL table. It then binds the ACL group to the port. Thus an ACL Table which has multiple ports in member list will be pointed by multiple ACL group members.

The ACL group/member are not exposed to the user configuration.


The ACL configuration is processed by the Orchagent process. The OA process has the AclOrch class that subscribes to ACL config changes in Redis ConfigDB/AppDB. AclOrch parses the ACL configuration and adds equivalent entries to the AsicDB. Syncd then picks up the AsicDB messages and calls libsai APIs to program the Barefoot ASIC.

More details are in SONiC ACL HLD mentioned in reference section.


These are the key files:
```
sonic-swss/orchagent/aclorch.cpp
sonic-swss/orchagent/aclorch.h
sonic-swss/orchagent/acltable.h
```



### SAI

Following are the SAI object types defined for the ACL:

SAI/inc/saitypes.h
```
    SAI_OBJECT_TYPE_ACL_TABLE    
    SAI_OBJECT_TYPE_ACL_ENTRY
    SAI_OBJECT_TYPE_ACL_COUNTER
    SAI_OBJECT_TYPE_ACL_RANGE
    SAI_OBJECT_TYPE_ACL_TABLE_GROUP
    SAI_OBJECT_TYPE_ACL_TABLE_GROUP_MEMBER
```

Syncd uses the `sai_api_query()` for SAI_API_ACL to obtain the CRUD function table for ACL.

SAI/inc/saiacl.h
```
typedef struct _sai_acl_api_t
{
    sai_create_acl_table_fn                     create_acl_table;
    sai_remove_acl_table_fn                     remove_acl_table;
    sai_set_acl_table_attribute_fn              set_acl_table_attribute;
    sai_get_acl_table_attribute_fn              get_acl_table_attribute;
    sai_create_acl_entry_fn                     create_acl_entry;
    sai_remove_acl_entry_fn                     remove_acl_entry;
    sai_set_acl_entry_attribute_fn              set_acl_entry_attribute;
    sai_get_acl_entry_attribute_fn              get_acl_entry_attribute;
    sai_create_acl_counter_fn                   create_acl_counter;
    sai_remove_acl_counter_fn                   remove_acl_counter;
    sai_set_acl_counter_attribute_fn            set_acl_counter_attribute;
    sai_get_acl_counter_attribute_fn            get_acl_counter_attribute;
    sai_create_acl_range_fn                     create_acl_range;
    sai_remove_acl_range_fn                     remove_acl_range;
    sai_set_acl_range_attribute_fn              set_acl_range_attribute;
    sai_get_acl_range_attribute_fn              get_acl_range_attribute;
    sai_create_acl_table_group_fn               create_acl_table_group;
    sai_remove_acl_table_group_fn               remove_acl_table_group;
    sai_set_acl_table_group_attribute_fn        set_acl_table_group_attribute;
    sai_get_acl_table_group_attribute_fn        get_acl_table_group_attribute;
    sai_create_acl_table_group_member_fn        create_acl_table_group_member;
    sai_remove_acl_table_group_member_fn        remove_acl_table_group_member;
    sai_set_acl_table_group_member_attribute_fn set_acl_table_group_member_attribute;
    sai_get_acl_table_group_member_attribute_fn get_acl_table_group_member_attribute;
} sai_acl_api_t;
```
The complete list of attributes can be found in the same file.

In addition, when an ACL is associated with a bindpoint like switch, this information is updated through a SET. In the case of Port, this is available here:


SAI/inc/saiport.h
```
    /**
     * @brief Port bind point for ingress ACL object
     *
     * Bind (or unbind) an ingress ACL table or ACL group on a port.
     * Enable/Update ingress ACL table or ACL group filtering by assigning
     * a valid object id. Disable ingress filtering by assigning
     * SAI_NULL_OBJECT_ID in the attribute value.
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_ACL_TABLE, SAI_OBJECT_TYPE_ACL_TABLE_GROUP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_PORT_ATTR_INGRESS_ACL,

    /**
     * @brief Port bind point for egress ACL object
     *
     * Bind (or unbind) an egress ACL tables or ACL group on a port.
     * Enable/Update egress ACL table or ACL group filtering by assigning
     * a valid object id. Disable egress filtering by assigning
     * SAI_NULL_OBJECT_ID in the attribute value.
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_ACL_TABLE, SAI_OBJECT_TYPE_ACL_TABLE_GROUP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_PORT_ATTR_EGRESS_ACL,
```


And for the Switch level, it is defined here:

SAI/inc/saiswitch.h
```
    /**
     * @brief Switch/Global bind point for ingress ACL object
     *
     * Bind (or unbind) an ingress ACL table or ACL group globally. Enable/Update
     * ingress ACL table or ACL group filtering by assigning a valid
     * object id. Disable ingress filtering by assigning SAI_NULL_OBJECT_ID
     * in the attribute value.
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_ACL_TABLE, SAI_OBJECT_TYPE_ACL_TABLE_GROUP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_SWITCH_ATTR_INGRESS_ACL,

    /**
     * @brief Switch/Global bind point for egress ACL object
     *
     * Bind (or unbind) an egress ACL tables or ACL group globally. Enable/Update
     * egress ACL table or ACL group filtering by assigning a valid
     * object id. Disable egress filtering by assigning SAI_NULL_OBJECT_ID
     * in the attribute value.
     *
     * @type sai_object_id_t
     * @flags CREATE_AND_SET
     * @objects SAI_OBJECT_TYPE_ACL_TABLE, SAI_OBJECT_TYPE_ACL_TABLE_GROUP
     * @allownull true
     * @default SAI_NULL_OBJECT_ID
     */
    SAI_SWITCH_ATTR_EGRESS_ACL,
```


### BF-SAI
Barefoot SAI layer is implemented in submodules/bf-switch/sai/saiacl.cpp

```
sai_acl_api_t acl_api = {
  create_acl_table : sai_create_acl_table,
  remove_acl_table : sai_remove_acl_table,
  set_acl_table_attribute : NULL,
  get_acl_table_attribute : sai_get_acl_table_attribute,
  create_acl_entry : sai_create_acl_entry,
  remove_acl_entry : sai_remove_acl_entry,
  set_acl_entry_attribute : sai_set_acl_entry_attribute,
  get_acl_entry_attribute : sai_get_acl_entry_attribute,
  create_acl_counter : sai_create_acl_counter,
  remove_acl_counter : sai_remove_acl_counter,
  set_acl_counter_attribute : sai_set_acl_counter_attribute,
  get_acl_counter_attribute : sai_get_acl_counter_attribute,
  create_acl_range : sai_create_acl_range,
  remove_acl_range : sai_remove_acl_range,
  set_acl_range_attribute : sai_set_acl_range_attribute,
  get_acl_range_attribute : sai_get_acl_range_attribute,
  create_acl_table_group : sai_create_acl_group,
  remove_acl_table_group : sai_remove_acl_group,
  set_acl_table_group_attribute : sai_set_acl_table_group_attribute,
  get_acl_table_group_attribute : sai_get_acl_table_group_attribute,
  create_acl_table_group_member : sai_create_acl_group_member,
  remove_acl_table_group_member : sai_remove_acl_group_member,
  set_acl_table_group_member_attribute : sai_set_acl_table_group_member_attribute,
  get_acl_table_group_member_attribute : sai_get_acl_table_group_member_attribute
};
```


The list of SAI attributes supported for each of the SAI objects as well as mappings from SAI to Switch attributes can also be found in the same file.


The automatic conversion of SAI attributes to Switch attributes mapping for ACL is defined in following map files:

```
sai/maps/saiacl_table.json
sai/maps/saiacl_entry.json
sai/maps/saiacl_group.json
sai/maps/saiacl_group_member.json
sai/maps/saiacl_counter.json
sai/maps/saiacl_range.json
```

The decision to place an ACL entry to the appropriate hardware (P4) table depends on the ACL table attributes as well as the ACL entry attributes. This deciscion can get deferred until the ACL entry is configured. Initially when the ACL table is configured, the table type is determined using the function sai_acl_tbl_type_get_from_attributes() in sai_create_acl_table() function. However this table type can get updated based on the ACL entry attributes. This is done at sai_acl_table_type_update() when creating first ACL entry in sai_create_acl_entry() function.



### BMAI
BF-SAI uses the BF-Switch CRUD APIs on the Switch "user" objects defined in `schema/appObj/acl.json`

The "user" objects supported for ACL are the following. This list is similar to the list of SAI objects.
```
acl_table
acl_entry
acl_group
acl_group_member
acl_counter
acl_range
```

The following ACL table types are supported in schema: MAC, IPV4, IPV6, etc. The detailed list is in the schema file. Bindpoints supported are PORT, LAG, VLAN, RIF and SWITCH. Not all table types/bindpoints are supported by every P4 profile. The comprehensive list of supported attributes are in the schema file.

A label mechanism is used internally to represent an acl table or acl group. A label is allocated per SAI ACL table. The same label is then assigned to the port. In the case of SAI ACL group with multiple member ACL tables, the allocated label value contains a combination of labels for all the member ACL tables. The exact bit position and width for each of the ACL tables in the combined label is specified in `api/switch_tna/acl.h`

```
// port_lag_label bitmap - unique label space for data ACLs
// Ingress
//        IPv4           IPv6       DSCP-Mirror Mirror      IFA
// |+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|+-+-+-+-+-+|+-+-+-+|+-+-+-+-+-+-+-+|
//  0               8              16         20      24              32
```

If there are multiple ports that separately bind to the same ACL group, then all of them would be assigned the same label.


An ACL entry can also use similar label mechanism to map a set of ports to match a unique ACL entry. The label is allocated per ACL entry.  And all the ports that needs to match that ACL entry are assigned the same label. Please see:
```
// in_ports_group_label bitmap -
// Ingress
//        IPv4           IPv6                Mirror
// |+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|+-+-+-+-+-+-+-+|
//  0               8              16              24
//
```


The BF-Switch ACL implementation is in `api/switch_tna/acl.cpp`. This file has routines for the "auto" objects created by acl_factory defined in `schema/asicObj/acl.json`. The object 'acl_hw_entry' derives from the 'acl_entry_object' and 'p4_object_match_action' and represents one entry in the ACL hardware table.






### P4
ACL is supported for both Tofino1 and Tofino2 profiles.

The shared ACL P4 code is in:

`p4src/shared/acl.p4`

Following type of ACL tables are supported. The keys used for the lookup can be found in the P4 file.

PreIngressAcl
IngressMacAcl
IngressIpv4Acl
IngressIpv6Acl
IngressIpAcl
IngressTosMirrorAcl

EgressMacAcl
EgressIpv4Acl
EgressIpv6Acl
EgressTosMirrorAcl

In addition, `p4src/shared/acl2.p4` also defines a few different kind of ACLs like
IngressQosAcl
IngressMirrorAcl
IngressPbrAcl
EgressQosAcl
EgressMirrorAcl




## Unit Test
The SAI unit test for ACL cvovers the basic CRUD APIs and are defined in `ptf/saiv2/saiacl.py`.
The Switch API unit tests can be found in `ptf/api/switch_acl.py`.


## Debugging
These are some of the commands available at SONiC level:
```
show acl table
show acl rule
aclshow
```

The following BMAI commands are available:
```
show acl_table
show acl_entry
show acl_group
show acl_group_member
show acl_counter
show acl_range

show port (ingress_acl_handle and egress_acl_handle points to acl group)
show vlan
show rif
show device
show counter acl_entry (Counters per entry)
show table_utilization (on ACL table usage)
```


## References
1. [SONiC ACL Configuration](https://github.com/sonic-net/SONiC/wiki/Configuration)
2. [SONiC ACL HLD](https://github.com/sonic-net/SONiC/blob/master/doc/acl/ACL-High-Level-Design.md)
3. [SAI ACL Proposal](https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Proposal-ACL-1.md)
4. [BMAI dev guide](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/blob/master/docs/bmai_developer_guide.md)

