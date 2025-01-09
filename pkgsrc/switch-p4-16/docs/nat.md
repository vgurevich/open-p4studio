# Network Address Translation (NAT)
This document describes the NAT feature for SONiC and integration with Barefoot stack.


## Overview


NAT or Network Address Translation is used to translated source/destination IP addresses or transport layer port numbers. This is standardized with IETF RFC 2663, RFC 3022, etc

In SONiC, NAT is supported for both Static NAT connections (configuration based) and Dynamic NAT connections (using a pool of addresses based on learning of new connections).

SONiC supports following NAT types:

1. Source-NAT/NAPT where only the source address/ports are translated.
2. Destination-NAT/NAPT where only the destination address/port are translated.
3. Twice or Double NAT/NAPT where both source/destination address/port are translated.

SONiC uses NAT zones configured at L3 interface level. When the traffic crosses from one zone to another NAT is performed.

SONiC keeps all the higher-level configuration required for NAT functionality and makes sure the Linux kernel and the SDK/Hardware are in sync with the NOS. In the Linux kernel, the conntrack subsystem keeps track of the NAT configuration and typically uses a single conntrack entry for both the in-bound and out-bound traffic for a bidrectional flow. In SDK/Hardware, there are 2 NAT entires (one for the Source NAT/NAPT and another for the Destination NAT/NAPT) for the same bidrectional flow.

The static NAT entries in hardware are not aged out even when there is no traffic on those connections. They are explicitly removed when the configuration is removed. On the other hand, the dynamic  NAT entries are removed periodically by using one of the 2 mechanisms:

1. Querying using a hitbit timer. Once a flow is inactive for hitbit duration, SONiC deletes the NAT entry.
2. Using SAI notifications: The SDK times out the inactive NAT entry and initiates the deletion.

## Configuration

SONiC NAT feature is enabled using the following CLIs. This starts the NAT container and the associated NAT services:
```
config nat feature enable
config feature state nat enabled
```
Once enabled, the feature status can be checked using the following CLI:
```
show feature config nat
show feature status nat
```
The NAT feature can be disabled and the NAT container stopped with following CLI:
```
config feature state nat disabled
config nat feature disable
```

NAT static entries are configured with the following CLI:
```
config nat add static basic <global_ip> <local_ip>
```
For example:
`config nat add static basic 10.10.10.10 1.1.1.1`

and to remove
 
`config nat remove static basic 10.10.10.10 1.1.1.1`

NAPT static entries are configured with the following CLI:
```
config nat add static tcp <global_ip> <global_port> <local_ip> <local_port>
config nat add static udp <global_ip> <global_port> <local_ip> <local_port>
```
For example:
`config nat add static tcp 20.20.20.20 2000 3.3.3.3 30000`

and to remove:

`config nat remove static tcp 20.20.20.20 2000 3.3.3.3 30000`


Configuring the dynamic NAT/NAPT requires the NAT address pool to be configured first. This is done using:
```
config nat add pool <pool_name> <global_ip_range> <global_port_range>
```
For example:
`config nat add pool nat_pool_1 20.20.20.20 2000-42000`

sets up a dynamic pool of 40,000 NAT connections.

In order to identify the packets that should be NATed, SONiC uses an ACL configuration. The ACL is linked to the NAT pool using the CLI:

`config nat add binding <binding_name> <pool_name> <acl_name>`

For example:
```
config acl add table -s ingress nat_acl_1 -p Ethernet72 -d "NAT ACL"
config nat add binding nat_binding_1 nat_pool nat_acl
```

The following commands can be used to remove dynamic NAT configuration:
```
config nat remove binding <binding_name>
config nat remove pool <pool_name>
config acl remove table <table_name>
```

The NAT is performed when a packet crosses from one NAT zone to another. In order to create these NAT zones, the interface is configured with:
```
config nat add interface [OPTIONS] <interface_name>
```
For example:

`config nat add interface -nat_zone 1 Ethernet80`

and to remove 

`config nat remove interface Ethernet80`

This will keep the interface in default nat_zone 0.

The complete configuration commands list can be found in the NAT SONiC HLD.

## Design
### SONiC
In SONiC, the NAT feature is part of "nat" container. This container has 2 processes natmgrd and natsyncd. Natmgrd process is responsible for reading the NAT configs from the Redis ConfigDB and setting up the NAT static entries in hardware by adding them to Redis AppDB. Natsyncd process on the otherhand mostly deals with the dynamic NAT configuration by listening to kernel conntrack events, etc. and setting up the NAT dynamic entries in Redis AppDB.

Once the NAT entries are added to AppDB (either by natmgrd or natsyncd), Natorch module in Orchagent picks them up. The Natorch is responsible for receiving and processing the AppDB messages and writing to the ASIC-DB to notify Syncd.  Syncd then picks up the ASIC-DB messages and calls libsai APIs to program the Barefoot ASIC.

SONiC design is mentioned in detail in SONiC NAT HLD. These are the key files:

Natmgrd

sonic-swss/cfgmgr/natmgrd.cpp
sonic-swss/cfgmgr/natmgr.cpp
sonic-swss/cfgmgr/natmgr.h

Natsyncd

sonic-swss/natsyncd/natsyncd.cpp
sonic-swss/natsyncd/natsync.cpp
sonic-swss/natsyncd/natsync.h

Natorch

sonic-swss/orchagent/natorch.cpp
sonic-swss/orchagent/natorch.h




### SAI
Syncd uses the `sai_api_query()` for SAI_API_NAT to obtain the CRUD function table for NAT.

SAI/inc/sainat.h
```
typedef struct _sai_nat_api_t
{
    sai_create_nat_entry_fn                   create_nat_entry;
    sai_remove_nat_entry_fn                   remove_nat_entry;
    sai_set_nat_entry_attribute_fn            set_nat_entry_attribute;
    sai_get_nat_entry_attribute_fn            get_nat_entry_attribute;

    sai_bulk_create_nat_entry_fn              create_nat_entries;
    sai_bulk_remove_nat_entry_fn              remove_nat_entries;
    sai_bulk_set_nat_entry_attribute_fn       set_nat_entries_attribute;
    sai_bulk_get_nat_entry_attribute_fn       get_nat_entries_attribute;

    sai_create_nat_zone_counter_fn            create_nat_zone_counter;
    sai_remove_nat_zone_counter_fn            remove_nat_zone_counter;
    sai_set_nat_zone_counter_attribute_fn     set_nat_zone_counter_attribute;
    sai_get_nat_zone_counter_attribute_fn     get_nat_zone_counter_attribute;
} sai_nat_api_t;
```

The CRUS APIs use the sai_nat_entry_t struct defined as follows to identify a NAT entry.
```
typedef struct _sai_nat_entry_t
{
    sai_object_id_t switch_id;
    sai_object_id_t vr_id;
    sai_nat_type_t nat_type;
    sai_nat_entry_data_t data;
} sai_nat_entry_t;
```

NAT supports only the default VRF 1. The nat_type can be either SNAT, DNAT, DoubleNAT or DNAT pool types. The sai_nat_entry_data_t defines the 5-tuple (sip, dip, proto, sport, dport) key for NAT entry lookup. The port translation is decided based on the key having a non-zero value for sport in the case of SNAPT, or a non-zero value for dport in the case of DNAPT. The sai_nat_entry_attr_t defines the attributes for the translated sip, translated dip, etc.

### BF-SAI
Barefoot SAI layer supports both the bulk and the non-bulk NAT SAI APIs. The NAT zone counter APIs are not supported currently (why?)

These are implemented in submodules/bf-switch/sai/sainat.cpp
'''
sai_nat_api_t nat_api =
{
  create_nat_entry : sai_create_nat_entry,
  remove_nat_entry : sai_remove_nat_entry,
  set_nat_entry_attribute : sai_set_nat_entry,
  get_nat_entry_attribute : sai_get_nat_entry,
  create_nat_entries : sai_create_nat_entries,
  remove_nat_entries : sai_remove_nat_entries,
  set_nat_entries_attribute : sai_set_nat_entries_attribute,
  get_nat_entries_attribute : sai_get_nat_entries_attribute,
};
'''

This is the list of NAT attributes:

SAI NAT attribute                        | Supported | Notes
-----------------------------------------|-----------|-------
SAI_NAT_ENTRY_ATTR_NAT_TYPE              | No        | Unused
SAI_NAT_ENTRY_ATTR_SRC_IP                | Yes       |
SAI_NAT_ENTRY_ATTR_SRC_IP_MASK           | No        | Partial not supported?
SAI_NAT_ENTRY_ATTR_VR_ID                 | No        | Why?
SAI_NAT_ENTRY_ATTR_DST_IP                | Yes       |
SAI_NAT_ENTRY_ATTR_DST_IP_MASK           | No        | Partial not supported?
SAI_NAT_ENTRY_ATTR_L4_SRC_PORT           | Yes       |
SAI_NAT_ENTRY_ATTR_L4_DST_PORT           | Yes       |
SAI_NAT_ENTRY_ATTR_ENABLE_PACKET_COUNT   | No        | Disabling not supported?
SAI_NAT_ENTRY_ATTR_PACKET_COUNT          | Yes       |
SAI_NAT_ENTRY_ATTR_ENABLE_BYTE_COUNT     | No        | Disabling not supported?
SAI_NAT_ENTRY_ATTR_BYTE_COUNT            | Yes       |
SAI_NAT_ENTRY_ATTR_HIT_BIT_COR           | No        | Disabling not supported?
SAI_NAT_ENTRY_ATTR_HIT_BIT               | Yes       |
SAI_NAT_ENTRY_ATTR_AGING_TIME            | Yes       |



The SAI attributes to Switch attributes mapping for NAT is defined `sai/maps/sainat.json`.

BF-SAI uses the BF-Switch CRUD APIs on the Switch "user' object SWITCH_OBJECT_TYPE_NAT_ENTRY

### BMAI
The NAT schema for SWITCH_OBJECT_TYPE_NAT_ENTRY is defined in `schema/appObj/nat.json`. The NAT entry object uses the device, type and the IP 5-tuple as a key for uniquely identifying the NAT entry. The vr_id in the key is not used currently and assumed to be default VRF.

The BF-Switch NAT implementation is in `api/switch_tna/nat.cpp`. This file has routines for the "auto" objects created by nat_factory defined in `schema/asicObj/nat.json`. The following auto object types are supported: 

dnapt_index, dest_napt, dest_nat, snapt_index, source_napt, source_nat, flow_nat, flow_napt, rewrite_nat

The parent object is the nat_entry object for all the NAT auto objects. The NAT objects above maps to the P4 control blocks discussed in next section.


### P4
NAT is supported for the x2 profile on Tofino1 and y2 profile on Tofino2.

The shared NAT P4 code is in:

`p4src/shared/nat.p4`

The NAT is performed in the SwitchIngress path for the profiles:
```
p4src/tofino/switch_tofino_x2.p4
p4src/tofino/switch_tofino2_y2.p4
```

The ingress NAT zone is identified when the packet comes in. This is done by setting up the local_md.nat.ingress_zone in p4src/shared/port.p4. In the case of egress nat zone, the determination is based on the egress interface in nexthop.p4

The DNAT/DNAPT processing is done as part of the IngressNat control block while the SNAT/SNAPT processing is done in the SourceNAT control block. 

## Flow
### Initialization
Natmgrd reads the NAT configuration and calls SAI APIs to initialize NAT feature in SDK. The NAT zone information is communicated to SDK through the SAI attribute `SAI_ROUTER_INTERFACE_ATTR_NAT_ZONE_ID`. BMAI maps this to the SWITCH_RIF_ATTR_NAT_ZONE and sets up the zoning information using port.p4 and nexthop.p4.

The P4 System ACL tables are programmed with action to punt the NAT miss packets to the CPU. SONiC uses the `SAI_HOSTIF_TRAP_TYPE_SNAT_MISS` and `SAI_HOSTIF_TRAP_TYPE_DNAT_MISS` to setup this path. The COPP policy configured determines the rate at which these CPU packets are metered.

SONiC also registers for NAT callback (`sai_nat_event_notification_fn`) through the switch attribute `SAI_SWITCH_ATTR_NAT_EVENT_NOTIFY`.


### NAT entry create
The NAT entry creation can happen in two ways. It can either be statically configured or dynamically. The static configuration is the simpler one where the Natmgrd reads the NAT configs from the config DB and pushes NAT entires to AppDB and Linux kernel. In the case of dynamic NAT, the NAT miss packets are sent to the CPU through the hostif path. The linux conntrack creates connection entries for these packets. Natsycnd module listens fot the conntrack Netlink notifications and pushes the NAT entires to AppDB. Orchagent process these AppDB messages and converts them to ASIC-DB messages. Syncd then uses the SAI NAT `create_nat_entry` API to program the hardware. If syncd is running in bulk mode, the SAI bulk API `create_nat_entries` is used. The dynamic NAT entries are programmed with an optional aging interval attribute `SAI_NAT_ENTRY_ATTR_AGING_TIME` attribute.


### NAT entry delete
In the case of NAT entry deletion, the static entries are removed by Natmgrd when the configuration for this is removed. In the case of Dynamic NAT entries, there are two mechanisms:

1. Natorch maintains the timeout for each NAT entry. Natorch periodically polls SDK for nat_entry active state using the `SAI_NAT_ENTRY_ATTR_HIT_BIT` attribute. If there is no traffic seen during the polling interval SDK reports the entry as inactive. When the NAT entry inactivity exceeds the timeout, Natorch deletes the Entry through Syncd which calls the SAI `remove_nat_entry` API or 'remove_nat_entries` bulk API.
 
2. SDK maintains the timeout for each NAT entry based on the `SAI_NAT_ENTRY_ATTR_AGING_TIME` attribute. This is implemented using the `idle_timeout` in nat.p4 tables and is used for both source and destination NAT/NAPT entries. BMAI registers for callback from driver using the `idleTableNotifyModeSet` API in `bf-switch/api/switch_tna/nat.cpp`. The NAT entry is expired when the inactivity exceeds the timeout and the driver callback triggers BMAI to cleanup the NAT entry. The NAT entry expiry is notified to Syncd through the `sai_nat_event_notification_fn` callback. Syncd subsequently notifies Orchagent and Natorch deletes the NAT entry through Syncd calling `remove_nat_entry` API.



## Unit Test
The unit test for NAT cvovers the basic CRUD APIs and are defined in `saiv2/sainat.py`. The unit tests for the NAT bulk APIs will be supported in future once the UT framework has the bulking support.


## Debugging
These are some of the commands available at SONiC level:
```
show nat confg
show nat translations
show nat statistics
show acl table
show acl rule
```

The following Linux commands show the connection specific information:
```
iptables -L -t mangle
iptables -L -t nat
conntrack -L -j
```

The following BMAI commands are available:
```
show nat_entry all
show nat_entry handle <hdl>
show rif (provides  nat_zone information for the RIF)
show hostif_trap (shows SNAT_MISS and DNAT_MISS information)
show hostif_trap_group
show acl (DNAT pool ACL)
show meter
show counter nat_entry, hostif, etc.
```


## References
1. [SONiC NAT Design](https://github.com/Azure/SONiC/blob/master/doc/nat/nat_design_spec.md#43-dynamic-napt-entry-aging-flow)
2. [SAI NAT](https://github.com/opencomputeproject/SAI/blob/master/doc/NAT/SAI-NAT-API.md)
3. [BMAI dev guide](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/blob/master/docs/bmai_developer_guide.md)
