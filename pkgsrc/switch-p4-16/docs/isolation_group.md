# SAI isolation group HLD

<!-- # Table of Contents
- **[MC-LAG high level design for SONiC](#MC-LAG-high-level-design-for-SONiC)**
- **[1. Document History](#1-Document-History)**
- **[2. SONiC Requirements](#2-SONiC-Requirements)**
    - **[2.1 ACL port isolation](#21-ACL-for-port-isolation)**
    - **[2.2 SAI isolationgroup for port isolation](#22-SAI-isolationgroup-for-port-isolation)**
- **[3. Implementation propositions](#3-Implementation-propositions)**
    - **[3.1 Port isolation by ACL](#31-Port-isolation-by-ACL)**
        - **[3.1.1 Egress ACL with FIELD_IN_PORT](#311-Egress-ACL-with-FIELD_IN_PORT)**
        - **[3.1.2 Ingress ACL with FIELD_OUT_PORTS](#312-Ingress-ACL-with-FIELD_OUT_PORTS)**
    - **[3.2 Port isolation by SAI isolationgroup](#32-Port-isolation-by-SAI-isolationgroup)**
        - **[3.2.1 Simple implementation for bridge port isolation group](#321-Simple-implementation-for-bridge-port-isolation-group)**
        - **[3.2.2 Full implementation for bridge port isolation group](#322-Full-implementation-for-bridge-port-isolation-group)** -->

# Document History

| Version | Date       | Author                 | Description                                      |
|---------|------------|------------------------|--------------------------------------------------|
| v.01    | 12/10/2021 | Myron Sosyak           | Initial version                                  |

# SAI Requirements
- [SAI Isolation Groups HLD](https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Isolation-Group.md)
- [saiisolationgroup.h](https://github.com/opencomputeproject/SAI/blob/master/inc/saiisolationgroup.h)

From SAI HLD:
>## Isolation Group Type
>
>There are two types of Isolation Groups:
>1. Port Isolation Groups
>2. Bridge-Port Isolation Groups
>
>### Port Isolation Group
>Port Isolation Groups are used to prevent traffic coming  on a physical port from going out of set of physical ports. They would prevent both >bridged and routed packets from going out via the physical port. The members of Port Isolation Groups would be port objects. Port Isolation Group >can be applied only on a port object.
>
>### Bridge-Port Isolation Group
>Bridge-Port Isolation Groups are used to prevent traffic coming  on a Bridge-Port from going out of set of Bridge-Ports. They would prevent only >Bridged (L2 Switched) packets from going out of other bridged ports. The members of Bridge-Port Isolation Groups would be Bridge-Port objects. >Bridge-Port Isolation Group can be applied only on a Bridge-Port object.
>
>*Note: If port isolation group is applied on underlying physical port of bridge port and at the same time bridge port isolation group is applied >on the bridge port, then switched traffic would be prevented from being forwarded to the members of bridge port isolation group.
>Both switched and routed traffic would be prevented from being forwarded to the members of the port isolation group.*


# Implementation limitations
Current implementation fully supports `SAI_ISOLATION_GROUP_TYPE_PORT`. But for `SAI_ISOLATION_GROUP_TYPE_BRIDGE_PORT` only bridge ports of type `SAI_BRIDGE_PORT_TYPE_PORT` are supported.


# P4 code
In p4 isolation logic is implemented on `egress` by three tables.

The first table `egress_ingress_port_mapping` match the packet and configure `local_md.port_isolation_group` and `local_md.bport_isolation_group` if the isolation group is assigned to the port or bridge_port. This table represents `SAI_PORT_ATTR_ISOLATION_GROUP` and `SAI_BRIDGE_PORT_ATTR_ISOLATION_GROUP` attrs.

```cpp
    action set_ingress_port_properties(switch_isolation_group_t port_isolation_group, switch_isolation_group_t bport_isolation_group) {
        local_md.port_isolation_group = port_isolation_group;
        local_md.bport_isolation_group = bport_isolation_group;
    }

    table egress_ingress_port_mapping {
        key = {
            local_md.ingress_port : exact;
        }

        actions = {
            NoAction;
            set_ingress_port_properties;
        }

        const default_action = NoAction;
        size = table_size;
    }
```

The second table is `egress_port_isolation` and represents `isolation_group_member` object. This table match the `local_md.port_isolation_group` and `egress_port` and set `local_md.flags.isolation_packet_drop = true` if appropriate member is configured. The third table is `egress_bport_isolation` and also represents `isolation_group_member` object, but for bridge_port members. This table math the `local_md.bport_isolation_group`, `local_md.flags.routed` and `egress_port` and set `local_md.flags.isolation_packet_drop = true` if appropriate member is configured.
The `local_md.flags.routed` is needed to isolate only bridged traffic. See [SAI Requirements](#SAI-Requirements) for details.

```cpp
    action isolate_packet() {
        local_md.flags.isolation_packet_drop = true;
    }

    table egress_port_isolation {
        key = {
            eg_intr_md.egress_port : exact;
            local_md.port_isolation_group : exact;
        }

        actions = {
            NoAction;
            isolate_packet;
        }

        const default_action = NoAction;
        size = table_size;
    }

    table egress_bport_isolation {
        key = {
            eg_intr_md.egress_port : exact;
            local_md.flags.routed : exact;
            local_md.bport_isolation_group : exact;
        }

        actions = {
            NoAction;
            isolate_packet;
        }

        const default_action = NoAction;
        size = table_size;
    }

```

# BMAI/SAI mapping
Port isolation groups configuration is exposed via 2 objects, `isolation_group` and `isolation_group_member`. The object/attrs name are consistent with SAI names in the following way:
```json
// saiisolationgroup.json
{
   "isolation_group" : {
      "profile" : "Per profile info",
      "switch" : "isolation_group",
      "attributes" : {
         "type" : "type",
         "isolation_member_list" : "isolation_group_members"
      }
   }
}

// saiisolationgroup_member.json
{
   "isolation_group_member" : {
      "profile" : "Per profile info",
      "switch" : "isolation_group_member",
      "attributes" : {
         "isolation_group_id" : "isolation_group_handle",
         "isolation_object" : "handle"
      }
   }
}
```

## User Objects
- isolation_group - This is typically a place holder object and does not consume HW resources until group is not assigned to the port/bridge port

```json
{
    "isolation_group" : {
        "attributes" : {
            "device" : "Device handle",
            "type" : "Isolation group type",
            "isolation_group_members" : "List of isolation group members"
        }
    }
}
```

- isolation_group_member - This is typically a place holder object and consume HW resources right after creation

```json
{
    "isolation_group_member" : {
        "attributes" : {
            "device" : "Device handle",
            "handle" : "Port or bridge port handle",
            "isolation_group_handle" : "Isolation group handle"
        },
        "membership" : [
            {
                "object" : "isolation_group",
                "attribute" : "isolation_group_members"
            }
        ],
        "key_groups": [["device", "isolation_group_handle", "handle"]]
    }
}
```

## Auto Objects
- `egress_port_isolation`
  - Parent is `isolation_group_member`
  - This represents the `egress_port_isolation` P4 match action table
- `egress_bridge_port_isolation`
  - Parent is `isolation_group_member`
  - This represents the `egress_bport_isolation` P4 match action table
- `egress_ingress_port_mapping`
  - Parent is `port`
  - This represents the `egress_ingress_port_mapping` P4 match action table
- `isolation_group_helper`
  - Parent could be `port` or `bridge_port`
  - This is the helper auto object to program `egress_ingress_port_mapping` table manually

# API Implementation
Once user create `isolation_group` nothing special is done, just store the object.

Once `isolation_group_member` is created the `egress_port_isolation` will be triggered and will configure `egress_port_isolation` and `egress_bridge_port_isolation` to match appropriate egress port and isolation group. But based on member handle type the only one will have a affect. For both types of group the logic is the same. The reason for that is that in this implementation we support only `bridge_port` of type `SAI_BRIDGE_PORT_TYPE_PORT` and that allow us to match by underlying port(actually by it's dev_port value).

Once user assign `isolation_group` to the port or bridge port the `isolation_group_helper` is triggered. It will:
- In case parent is `port` - lookup for bridge port which is created over this port (or over lag in which this port is added)
- In case parent is `bridge_port` - get underlying port or ports in case bridge port is created over lag

...and collects all information about isolation group configuration in related ports and bridge port. Then for each port it will call `egress_ingress_port_mapping` with port as parent to program appropriate entries in the HW. We need information about port and bridge port isolation group configured to create appropriate entries for bridged and routed traffic. Regarding to SAI HLD note:
>*Note: If port isolation group is applied on underlying physical port of bridge port and at the same time bridge port isolation group is applied on the bridge port, then switched traffic would be prevented from being forwarded to the members of bridge port isolation group.
Both switched and routed traffic would be prevented from being forwarded to the members of the port isolation group.*


