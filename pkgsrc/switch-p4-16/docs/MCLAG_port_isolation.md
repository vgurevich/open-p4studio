# MC-LAG port isolation HLD

# Table of Contents
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
        - **[3.2.2 Full implementation for bridge port isolation group](#322-Full-implementation-for-bridge-port-isolation-group)**

# 1. Document History

| Version | Date       | Author                 | Description                                      |
|---------|------------|------------------------|--------------------------------------------------|
| v.01    | 02/12/2021 |Myron Sosyak            | Initial version                                  |

# 2. SONiC Requirements
To avoid L2 BUM traffic flooding in above topology sonic should some how isolate traffic recived on MC-LAG peer from MC-LAG members. To do that in [Sonic-mclag-hld](https://github.com/Azure/SONiC/blob/master/doc/mclag/Sonic-mclag-hld.md) and [MCLAG_Enhancements_HLD](https://github.com/Azure/SONiC/blob/master/doc/mclag/MCLAG_Enhancements_HLD.md) two approaches are defined.

From [Sonic-mclag-hld](https://github.com/Azure/SONiC/blob/master/doc/mclag/Sonic-mclag-hld.md):
> In ASIC, ACL rule is used to isolate peer link from MC-LAG port. The rule is when the traffic is received from peer link and the output port is MC-LAG member port, the traffic must be dropped. For the chips whose ACL rule can't support out-port, there is a workaround in SAI layer by combination of ingress acl and egress acl. An alternative approach is to use isolation group. But The approach of isolation group still has some weakness, Firstly isolation group can't support tunnel-port and orchagent has not isolation group logic currently. Secondly isolation group may not be supported by all ASIC vendors. Using ACL is a more generic way to support the isolation function. We will refine this function to use isolation group later if it’s required.

From [MCLAG_Enhancements_HLD](https://github.com/Azure/SONiC/blob/master/doc/mclag/MCLAG_Enhancements_HLD.md):
>3.3.4 Port Isolation changes
>
>In this enhancement, the port isolation logic is updated to use SAI Port Isolation instead of Egress ACL. Using isolation group, unwanted traffic can be dropped at the ingress, i.e. before it gets queued etc. This will give better performance as unnecessary traffic will filtered at ingress.
>
>MclagSyncd is enhanced to check the platform capabilities for Isolationgroup support, platform not supporting isolation group will continue to use existing approach of using ACL's.
>

>3.7.1 Port Isolation
>
>For controlling the BUM traffic received on the peer_link towards MCLAG portChannels, one isolation group is allocated of type SAI_ISOLATION_GROUP_TYPE_BRIDGE_PORT. All MCLAG PortChannel member ports get added to MLCAG isolation group , ports are set with attribute SAI_BRIDGE_PORT_ATTR_ISOLATION_GROUP.
>
>The following SAI definitions will be used and no enhancements necessary.
>
>- https://github.com/opencomputeproject/SAI/blob/master/inc/saiisolationgroup.h

## 2.1 ACL for port isolation
The first approach uses ACL. It creates `ingress` ACL table with `SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS` and bind it to peerlink interface. Then it creates ACL rule with `SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS=<list of members of MC-LAG member LAG>` to drop packet.

Requires:
- Ingress ACL table
- `SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS`
- `SAI_ACL_ENTRY_ATTR_FIELD_OUT_PORTS`

> **NOTE:**  Current SONiC implementation is broken. SONIC will create MCLAG ACL table with table type L3(predefined type in SONiC) witch do not have `SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS`


## 2.2 SAI isolationgroup for port isolation
The second approach uses SAI isolationgroup. SONiC will create `sai_isolation_group` of type `bridge port` and add MCLAG members as members of this group and set this group to peer link bridge port as `SAI_BRIDGE_PORT_ATTR_ISOLATION_GROUP`

Requires:
- `SAI ISOLATION GROUP` of type `SAI_ISOLATION_GROUP_TYPE_BRIDGE_PORT`
- Support `SAI_OBJECT_TYPE_BRIDGE_PORT` as `SAI_ISOLATION_GROUP_MEMBER_ATTR_ISOLATION_OBJECT`
- `SAI_BRIDGE_PORT_ATTR_ISOLATION_GROUP`

# 3. Implementation propositions
This sections contains propositions on how it could be implemented
## 3.1 Port isolation by ACL

ACL based propositions where refused by team.
>"We already have enough issues related to support of in/out_ports in ACLs and there is no reason to bring in additional complexity for MCLAG. So, let's use the bridge port isolation logic to implement pruning of unwanted traffic for MCLAG."

### 3.1.1 Egress ACL with FIELD_IN_PORT
#### In SONIC:
- Add new `TABLE_TYPE_MCLAG_EGRESS` like here [aclorch.cpp#L2791](https://github.com/Azure/sonic-swss/blob/bb0733aa67ffc4e430e70bcf2db2dc6316172b32/orchagent/aclorch.cpp#L2791) with LAG bind point type and `SAI_ACL_TABLE_ATTR_FIELD_IN_PORT`. After the latest changes in SONiC ACl it is possible to define table types on the fly. This could be done only for specific platform.

- Extend [MclagLink::setPortIsolate](https://github.com/Azure/sonic-swss/blob/master/mclagsyncd/mclaglink.cpp#L189) to create Egress ACL table with type `TABLE_TYPE_MCLAG_EGRESS` and bind it to all MCLAG members interfaces(LAG objects) as `egress_acl`. Then to isolate the traffic add ACL rule with `SAI_ACL_ENTRY_ATTR_FIELD_IN_PORT` == MCLAG PEER link port

#### In SDK:
- add new p4 table `mclagACL` with `local_md.egress_port_lag_label` (ACL binding to port/lag) and `local_md.ingress_port` match fields and map `TABLE_TYPE_MCLAG_EGRESS` to it.


### 3.1.2 Ingress ACL with FIELD_OUT_PORTS
#### In SONIC:
- Fix [MclagLink::setPortIsolate](https://github.com/Azure/sonic-swss/blob/master/mclagsyncd/mclaglink.cpp#L189) to create ACL table with type `TABLE_TYPE_MCLAG` instead of `TABLE_TYPE_L3`

#### In SDK:
- Extend `switch_local_metadata_t` with new field `isolation_label`
- Ignore `SAI_ACL_TABLE_ATTR_FIELD_OUT_PORTS` in table mapping. Map to `IngressIpAcl`
- Add new action `set_port_isolation_label(label)` to `IngressIpv4Acl`
- On rule creating:
    - calculate the `isolation_label` and create match all rule with action `set_port_isolation_label` and calculated label as action parameter
    - create one extra rule in `EgressSystemACL` table to match `isolation_label` with drop action.


## 3.2 Port isolation by SAI isolationgroup

From [SAI-Isolation-Group.md](https://github.com/opencomputeproject/SAI/blob/master/doc/SAI-Isolation-Group.md#isolation-group-type)

> There are two types of Isolation Groups:
> 1. Port Isolation Groups
> 2. Bridge-Port Isolation Groups
>
> ### Port Isolation Group
> Port Isolation Groups are used to prevent traffic coming  on a physical port from going out of set of physical ports. They would prevent both bridged and routed packets from going out via the physical port. The members of Port Isolation Groups would be port objects. Port Isolation Group can be applied only on a port object.
>
> ### Bridge-Port Isolation Group
> Bridge-Port Isolation Groups are used to prevent traffic coming  on a Bridge-Port from going out of set of Bridge-Ports. They would prevent only Bridged (L2 Switched) packets from going out of other bridged ports. The members of Bridge-Port Isolation Groups would be Bridge-Port objects. Bridge-Port Isolation Group can be applied only on a Bridge-Port object.
>
>> *Note: If port isolation group is applied on underlying physical port of bridge port and at the same time bridge port isolation group is applied on the bridge port, then switched traffic would be prevented from being forwarded to the members of bridge port isolation group.
>> Both switched and routed traffic would be prevented from being forwarded to the members of the port isolation group.*

MCLAG in SONiC do port isolation by SAI isolationgroup and requires implementation for bridge port isolation group. By SAI spec the main difference between port and bridge port isolation group is that the first isolate all traffic and the second isolate only bridged traffic. But this difference is not valuable for MCLAG, So I see two options.

### 3.2.1 Simple implementation for bridge port isolation group
The simple implementation is already added to the SDK by https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/2636. Please see [isolation_group.md](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/blob/master/docs/isolation_group.md)

### 3.2.2 Full implementation for bridge port isolation group
Ideally, we would drop unicast packets in the ingress pipeline and flooded/mulitcast packets in PRE so there is no buffer/link bw consumed for the dropped packets. If we want to keep the implementation simple, we can drop all unwanted packets in the egress pipe by adding a new P4 table that matches on ingress_port, egress_port, and routed flags.
