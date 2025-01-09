# ECMP

This document describes the ECMP functionality offered by BF Switch.

## Object Model

ECMP configuration is exposed via 2 objects, **ecmp** and **ecmp_member**. While the objects are named as ecmp, they can also be used to configure **wcmp**.

### User Objects
- ecmp
  - This is typically a place holder object and does not consume HW resources until members are added to the group

```json
"ecmp" {
    "attributes" {
        "device" : "Device Handle",
        "type" : "ECMP type, [ ordered, fine grain ]",
        "ecmp_members" : "Read-only list of ECMP members"
    }
}
```
- ecmp_member
  - First and last member objects of an ECMP group effects how the ECMP match tables and selectors HW resources are updated.
```json
"ecmp_member" {
    "attributes" {
        "device" : "Device Handle",
        "ecmp_handle" : "ECMP group object handle",
        "nexthop_hande" : "Nexthop handle for this member",
        "enable" : "Enable/disable flag per member",
        "weight" : "Weight for Weighted ECMP"
    }
}
```
> **"weight" is a create-only attribute and updating its value is not supported at this time**

### Auto Objects
- ecmp_table
  - Parent is "ecmp"
  - This represents the ECMP P4 match action table
- ecmp_selector_group
  - Parent is "ecmp"
  - This is the Action Selector group object for each ecmp group
- ecmp_selector
  - Parent is "ecmp_member"
  - This is the Action profile table for each ecmp member
- ecmp_membership
  - Parent is "ecmp_member"
  - This is the helper auto object to program above tables manually

> ECMP Hierarchy
```
            +----------------+
            | ecmp_sel_group |
+------+    +----------------+
| ecmp |----|
+------+    +------------+
            | ecmp_table |
            +------------+
```

> ECMP member Hierarchy
```
                   +-----------------+
                   | ecmp_membership |
+-------------+    +-----------------+
| ecmp_member |----|
+-------------+    +---------------+
                   | ecmp_selector |
                   +---------------+
```

## P4 Implementation
```c++
ActionProfile ecmp_action_profile;
ActionSelector ecmp_selector (max_group_size = 1024,
                              max_members = 64K,
                              action_profile = ecmp_action_profile);

action set_port(port_id_t port) {
    local_md.egress_port = port;
}
table ecmp {
    keys {
        local_md.nexthop : exact
    }
    implementation: action_selector
    size: 2048
    actions {
        set_port
        NoAction
    }
}
```
> ** A selector group is basically a list of member IDs where each member is a "copy" of physical action profile memory**

Assume,
 - ecmp MAU table ID = 1
 - ecmp selector group ID = 10
 - ecmp action profiles for port 1, 2 and 3 use member_ids 100, 200 and 300

The HW lookups will logically looks like this
```
local_md.nexthop == 1 -> ecmp_selector_group_id -> 10
ecmp_selector_group_id == 10 -> ecmp_selector_member_id 100
                                ecmp_selector_member_id 200
                                ecmp_selector_member_id 300
ecmp_action_profile_id == 100 -> port 1
ecmp_action_profile_id == 200 -> port 2
ecmp_action_profile_id == 300 -> port 3
```

### HW resource usage
The compiler allocates HW resources in the following manner for ActionSelectors and ActionProfiles.
 - max_group_size is the maximum size allowed for each ECMP group. The low-level driver assumes the default size of the group to be this size if not explicitly configured to a lower value.
 - max_members is the total number of member entries in all groups combined.

Assuming, the default maximum size is used for all groups, a theoretical maximum of 64 ECMP groups can be created. This is the logical view of ECMP groups and their members.
```
   1        2        3               64
[ 1024 ] [ 1024 ] [ 1024 ] ...... [ 1024 ]
```
In effect though, the picture is slightly different. As noted above, each selector member ID is a copy of the action profile entry. So every action profile entry created for each port above takes up a memory location. And each reference to this action profile entry in each group uses a copy of this action profile. Hence, in practice, only 63 ECMP groups can be created in total.
```
   1        2        3               63
[ 1024 ] [ 1024 ] [ 1024 ] ...... [ 1024 ] [ reserved for action profiles]
```

This becomes less resource friendly when it comes to WCMP. A selector group does not allow using duplicate member IDs in each group. **This is not a HW limitation but a limitation of the low-level driver.** For this reason, we end up creating action profile entries for each port proportional to the weight of the member. This means, if the weight of an ecmp member is defined as 100, then 100 action profile entries have to be created with all of them giving the same egress port. And if there are 3 members each with weight 100, 300 action profile entries have to be created instead of just 3 in an ideal scenario.

## API Implementation
The API implementation starts off with a default size of 64 and grows the group size as more members are added to the group.

The API implementation is mostly driven through the ecmp_membership object. The reason for this is mainly because of the HW implementation of an ActionSelector. **A match action table entry cannot refer to an empty Selector group**. This drives the order in which the above P4 tables are programmed.

The following sequences are listed.
> Create
 1. ECMP group created. MAU table entry add referring to this ECMP group. ECMP members added to ECMP group.
 2. ECMP group created. ECMP members added to ECMP group. MAU table entry added referring to this ECMP group.
> Delete
 1. MAU table entry deleted. ECMP members deleted. ECMP group deleted.
 2. ECMP members deleted. MAU table entry deleted. ECMP group deleted.
 3. ECMP group deleted while MAU table entry is present. This is an invalid operation and not supported. Listed here only for completeness.

### ECMP MAU table
To be able to add an MAU table entry referring to an empty ECMP group, a workaround is used when creating the entries. A default entry is created for the ECMP ActionProfile which is a dummy entry with $ACTION_MEMBER_ID = 0xFFFFFFFF and its action set to NoAction. When an ECMP group is created, the selector group will use this dummy action profile as a member. This way the ECMP selector group entry in the HW is never empty. An MAU table entry can always point to an ECMP group without having to worry if the ECMP selector is empty. Conversely, when the ECMP group referred to by an MAU entry becomes empty, the selector will revert to using the dummy action profile entry and the MAU table entry remains valid.

### ECMP Selector group
The above explanation applies here. During the ecmp_selector_group auto object evaluation, the code checks if the ecmp_members size is 0. If the size is 0, then the dummy action profile entry is added as a member. When a new valid member is added to the ecmp group, the selector group object is re-evaulated from the ecmp_membership class. The opposite is also true when the ecmp group is about to become empty.

### ECMP Selector (ActionProfile)
This object has "ecmp_member" as a parent but its class implementation is not registered with S3. This class is instantiated manually from the "ecmp_membership" class.

### ECMP membership helper
The major chunk of ECMP operation is implemented in this class. This class is responsible for making sure the ECMP selector is updated when it moves from empty to non-empty or vice-versa. Any updates to the ECMP member are also handled from this class. Finally, the WCMP logic is also driven from this class.

#### ACTION MEMBER_ID Calculation
ECMP member OID is a 64 bit value with MSB 16 representing object type and LSB 48 bits representing the ID value.

weight is the weight attribute of the EMCP member with default value of 1.
```
[ (object_id << 16) + range(0, weight) ]

For an ECMP member, this means
(1 << 16) + 1 = 0x10001

For a WCMP object with weight 4, this means
(1 << 16) + 1 = 0x10001
(1 << 16) + 2 = 0x10002
(1 << 16) + 3 = 0x10003
(1 << 16) + 4 = 0x10004
```
> **This effectively means ECMP is actually WCMP with member weights of 1**

The following subsections explain this class behavior for create, update and delete scenarios using below object IDs as example
```
- ECMP group OID    -> 1
- ECMP member 1 OID -> 1, weight -> 2
- ECMP member 2 OID -> 2, weight -> 3
- ECMP member 3 OID -> 3, weight -> 4
```
> Create

When the first member(1) is added
 - Action Profile entries are created with $ACTION_MEMBER_ID 0x10001, 0x10002
 - Selector group updated with
```
$ARRAY  [ 0x10001, 0x10002 ]
$STATUS [ True,    True ]
```
 - Selector group moves from dummy array to above list

When members 2, 3 are added
 - Action Profile entries are created with $ACTION_MEMBER_ID
   - 0x20001, 0x20002 and 0x20003 for member 2
   - 0x30001, 0x30002, 0x30003 and 0x30004 for member 3
   - Selector group updated with
```
$ARRAY  [ 0x10001, 0x10002, 0x20001, 0x20002, 0x20003, 0x30001, 0x30002, 0x30003, 0x30004 ]
$STATUS [ True,    True,    True,    True,    True,    True,    True,    True,    True ]
```

> Update

When member 2 is disabled
 - Action Profile entries are updated with $ACTION_MEMBER_ID
   - No change in ActionProfile entries
   - Selector group updated with
```
$ARRAY  [ 0x10001, 0x10002, 0x20001, 0x20002, 0x20003, 0x30001, 0x30002, 0x30003, 0x30004 ]
$STATUS [ True,    True,    False,   False,   False,   True,    True,    True,    True ]
```
> Delete

When members 2 and 3 are deleted
 - Selector group is updated with
```
$ARRAY  [ 0x10001, 0x10002 ]
$STATUS [ True,    True ]
```
 - Action Profile entries with $ACTION_MEMBER_ID 0x20001, 0x20002, 0x20003, 0x30001, 0x30002, 0x30003 and 0x30004 are deleted

When last member 1 is deleted
 - Selector group moves back to using dummy array
 - Action Profile entry with $ACTION_MEMBER_ID 0x20001, 0x20002 are deleted

## ECMP Hashing
TBD
