# Multicast HLD

\* _All changes described in this document that compare new code to some older pieces consider the state of bf-switch repository that was current on the day before multicast re-enabling works started. Refer https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/tree/HEAD@%7B2022-07-01%7D if needed._

## Packet Replication Engine

Packet Replication Engine (PRE) as a part of Traffic Manager (TM) is a fixed function device which supports multicast forwarding with source pruning support. It is designed to handle 2-level multicast with multiple networks and multiple ports per network. It fits 2-level multicast schema shown in Fig1.

```
                    +---------------+
                    | 2-level group |               (pre.mgid)
                    +---------------+
                    /       |        \
                   /        |         \
           +-------+    +-------+    +-------+
           | rid_1 |    | rid_2 | .. | rid_n |      Level1 nodes
           +-------+    +-------+    +-------+      (pre.node)
           /   |   \       /  \        /    \
          /    |    +--+  /    +------/---+  +
         /     |        \/           /     \ |
    +-----+  +-----+  +-----+  +-----+    +-----+   Level2 nodes
    | p_0 |  | p_1 |  | p_2 |  | p_3 | .. | p_n |   ($MULTICAST_LAG_ID/$DEV_PORT
    +-----+  +-----+  +-----+  +-----+    +-----+   arrays in pre.node)
```
*Fig1: 2-level Multicast schema*

Level1 node represents Replication ID (RID). RID is an abstraction of a network. Each network is a 1-level group. RID can be mapped as a VLAN or other replication instance. L2 multicast can be considered as multicast group that have single RID.

Level2 node is a port bitmap. It stores a list of ports (or LAGs) to which packet should be replicated.

To make use of this schema, we have to program:
- pre.mgid table – multicast group ID is responsible for level 1 replication – from the group to the nodes. Basing on mgid value it assigns *node_id*s and *node_l1_xid*s. It says to what (Level1) nodes a packet within given multicast group should be replicated,

```
bfrt.pre> mgid.info
--------> mgid.info()
Table Name: mgid
Full Name: pre.mgid
Type: PRE_MGID
Usage: 6
Capacity: 65536

Key Fields:
Name    Type      Size  Required    Read Only
------  ------  ------  ----------  -----------
$MGID   EXACT       16  True        False

Data Fields:
Name                          Type        Size  Required    Read Only
----------------------------  --------  ------  ----------  -----------
$MULTICAST_NODE_ID            INT_ARR       32  False       False
$MULTICAST_NODE_L1_XID_VALID  BOOL_ARR       1  False       False
$MULTICAST_NODE_L1_XID        INT_ARR       16  False       False
$MULTICAST_ECMP_ID            INT_ARR       32  False       False
$MULTICAST_ECMP_L1_XID_VALID  BOOL_ARR       1  False       False
$MULTICAST_ECMP_L1_XID        INT_ARR       16  False       False
```
- pre.node table – is responsible for mapping between RID values and device ports (or LAGs) to which a packet should be replicated,
```
bfrt.pre> node.info
--------> node.info()
Table Name: node
Full Name: pre.node
Type: PRE_NODE
Usage: 6
Capacity: 16777216

Key Fields:
Name                Type      Size  Required    Read Only
------------------  ------  ------  ----------  -----------
$MULTICAST_NODE_ID  EXACT       32  True        False

Data Fields:
Name               Type       Size  Required    Read Only
-----------------  -------  ------  ----------  -----------
$MULTICAST_RID     UINT64       16  True        False
$MULTICAST_LAG_ID  INT_ARR       8  True        False
$DEV_PORT          INT_ARR      32  True        False

```
- pre.prune table – prevents multicast copies from going back to the source. It maps l2_xid to a port. When a packet comes to the switch, we check its l2_xid and exclude corresponding port from the list of ports stored in Level1 node to which the packet will be copied. Exclusion ID’s may be read from metadata.
```
bfrt.pre> prune.info
--------> prune.info()
Table Name: prune
Full Name: pre.prune
Type: PRE_PRUNE
Usage: 288
Capacity: 288

Key Fields:
Name               Type      Size  Required    Read Only
-----------------  ------  ------  ----------  -----------
$MULTICAST_L2_XID  EXACT       16  True        False

Data Fields:
Name       Type       Size  Required    Read Only
---------  -------  ------  ----------  -----------
$DEV_PORT  INT_ARR      32  True        False
```

\* PRE supports 2 levels of pruning:
- Level 1 pruning removes entire Level1 node by l1_xid which may be assigned to each node.
- Level 2 pruning removes source port by RID and l2_xid which is remapped to a bitmap of ports (or LAGs).

After programming these tables we will expect that there is one mgid entry per each multicast group (either L2MC and IPMC). The details on how members are mapped to level1 and level2 nodes are described in [_Code changes_](#code-changes) section.

## P4 code

All data plane mechanisms strictly related to multicast functionalities are implemented multicast.p4 module.

Following controls determine group_id (mgid) for incoming packets:
- MulticastBridge[v6] for (S, G) and (*, G)
- MulticastRoute[v6] for (S, G) and (*, G)

Control *IngressMulticast* first triggers the *MulticastRoute* lookups. If all of the *MulticastRoute* lookups fail or the rpf check fails, then the *MulticastBridge* lookups are triggered. Finally the *fwd_result* table collects the results of the multicast lookups from local_md multicast-related values, and determines which action to take: *multicast_route* (packet is routed, mrpf is true), *multicast_bridge* (packet is not routed, mrpf may be either true or false), or *multicast_flood*. The *fwd_result* table is programmed by BMAI using constant entries programmed in the mcast_fwd_result class.

Control *MulticastFlooding* sets the multicast group ID for flooding, based on the bd, pkt_type, and flood_to_multicast_routers flag. If the ID is set, a packet will be multicasted to corresponding multicast group.

Control *Replication* sets *bd* and/or *nexthop* and *tunnel_nexthop* according to RID value. We can have one RID per VLAN or other replication instance that is an abstraction of a network.

## BMAI schema

All the multicast-related objects in BMAI can be split into three groups:
1. routing/bridging-related auto objects – *multicast_factory* with all *[ipv4/ipv6]\_multicast\_[route/bridge]_[s_g/x_g]* objects,
2. auto objects and internal user objects for PRE programming,
3. user objects for creating multicast groups, multicast group members, multicast routes, and multicast bridges.

User objects (3) correspond with SAI objects model to a great extent. They were adjusted to support customer specific SAI extensions.

SAI distinguishes between L2 (L2MC) and L3 (IPMC) multicast types. Previously, our model defined one *multicast_group* object which might be used for both IPMC and L2MC (distinguished by the type) and that stored a list of its members. However, the *type* attribute was not used anywhere (as we actually didn't support L2 multicast). To fit SAI requirements better, *multicast_group* has been split into two separate objects: *l2mc_group* and *ipmc_group* (see [_Code changes_](#code-changes) section for reference).

Similarly, the model previously defined one multicast_member user object. It has also been split into *l2mc_member* and *ipmc_member* to handle L2MC and IPMC separately.
Few things to notice:
- for both types we need to keep an equivalent of *multicast_group_handle* attribute which has been renamed to *l2mc_group_handle* and *ipmc_group_handle* respectively,
- SAI IPMC allows to use RIF or tunnel as SAI_IPMC_GROUP_MEMBER_ATTR_IPMC_OUTPUT_ID; this has been mapped to the *output_handle* attribute of *ipmc_member*,
- SAI L2MC permits only for bridge port objects to be used as SAI_L2MC_GROUP_MEMBER_ATTR_IPMC_OUTPUT_ID which are now being remapped to port or lag or tunnel before sending down from SAI to BMAI layer and setting it as *output_handle* of *l2mc_member*,
- for *ipmc_member* we needed to add extension attributes to handle custom SAI attributes requested by a customer; these are SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_L2MC_OUTPUT_ID and SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_EGRESS_OBJECT,
- old *vlan_id* and *vlan_handle* attributes was deprecated.

*multicast_route* user object was a parent of *multicast_factory* (and all multicast route/bridge objects). *multicast_factory* was used to choose the right type of route or bridge depending on settings passed by a user. However, all "routes" were only used for IPMC and all "bridges" only for L2MC purposes. And therefore, *multicast_route* has been split into *ipmc_route* and *l2mc_bridge* (both to be programmed by a user) and *multicast_route_factory* child auto object has been split into *multicast_route_factory* and *multicast_bridge_factory* which choose only from *[ipv4|ipv6]\_multicast_route_[s_g|x_g]* objects and *[ipv4|ipv6]\_multicast_bridge_[s_g|x_g]* objects respectively.

Old *multicast_route* object was intended to handle both L2MC and IPMC so after splitting it:
- *network_handle* attribute (that stored either VRF or VLAN handle) was changed to *vrf_handle* for *ipmc_route* and *vlan_handle* for *l2mc_bridge*,
- *pim_mode* and *rpf_group_handle* apply only to *ipmc_route*.

PRE-related objects that existed were:
- mc_mgid – it was created for each *multicast_group* and *vlan*, it programmed pre.mgid entry for them,
  - it sent down an multicast group identifier (a key) and a list of mc_node IDs (computed in the same way as in *mc_node* class),
- mc_node – it was created for each multicast member separately,
  - it sent down computed node ID (key), rid value, and a list with only one port or LAG,
  - this was not effective as we could create single node for all members in the same BD and use the same RID value for them,
- mc_node_vlan – created for each vlan to handle vlan flooding,
  - it sent down computed node ID (key), rid value, and a list of ports/LAGs belonging to the node (so all vlan members),
- mc_node_vlan_member – created for each *vlan_member* of type tunnel to handle L2 VxLAN flooding
  - it sent down computed node ID (key), rid value, and a list with only one port or LAG.

*mc_node* auto object is not in use anymore. It is replaced with relevant: *l2mc_node*, *mc_node_tunnel*, and *ipmc_node* objects (see [_Code changes_](#code-changes) section for reference).

## SAI
Customer expects us to handle two additional *ipmc_group_member* attributes:
- SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_L2MC_OUTPUT_ID
  - described as: “L2MC output id”
  <br> @type sai_object_id_t
  <br> @flags CREATE_ONLY
  <br> @objects SAI_OBJECT_TYPE_L2_MC_GROUP
  <br> @allownull true
  <br> @default SAI_NULL_OBJECT_ID
- SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_EGRESS_OBJECT
  - described as: “Egress object id”
  <br> @type sai_object_id_t
  <br> @flags CREATE_ONLY
  <br> @objects SAI_OBJECT_TYPE_NEXT_HOP, SAI_OBJECT_TYPE_NEXT_HOP_GROUP
  <br> @allownull true
  <br> @default SAI_NULL_OBJECT_ID

To support above requirements (mainly to extend current implementation to fit L2MC) we needed to implement:
- sail2mc.h – implemented in sail2mc.cpp where we handle CRUD operations for L2MC entries:
  - l2mc_entry is mapped to a relevant *l2mc_bridge* object,
  - SAI_L2MC_ENTRY_ATTR_OUTPUT_GROUP_ID is mapped to *group_handle*,
  - SAI_L2MC_ENTRY_ATTR_PACKET_ACTION  is not supported (i.e. the FORWARD action is applied regardless of the value),
  - l2mc_entry struct attributes are mapped to a new *l2mc_bridge* object that programs L2MC forwarding paths;
    - bv_id (bridge or vlan ID) -> vlan_handle,
    - type (one of SAI_L2MC_ENTRY_TYPE_SG or SAI_L2MC_ENTRY_TYPE_XG) -> is used to determine whether we program src_ip or set it to all zeros,
    - destination (IP address) -> grp_ip,
    - source (IP address) -> src_ip (programmed only for **_multicast_bridge_s_g*),
- sail2mcgroup.h – is implemented in l2mcgroup.cpp where we create L2 multicast groups with a set of members:
  - SAI_OBJECT_TYPE_L2MC_GROUP is mapped to *l2mc_group*,
  - SAI_L2MC_GROUP_ATTR_IPMC_MEMBER_LIST is mapped to SWITCH_L2MC_GROUP_ATTR_L2MC_MEMBERS,
  - SAI_L2MC_GROUP_ATTR_L2MC_OUTPUT_COUNT which stores number of L2MC output in the group is not supported,
  - l2mc_group_member is mapped to *l2mc_member*,
  - SAI_L2MC_GROUP_MEMBER_ATTR_L2MC_GROUP_ID is mapped to SWITCH_L2MC_MEMBER_ATTR_L2MC_GROUP_HANDLE,
  - SAI_L2MC_GROUP_MEMBER_ATTR_L2MC_OUTPUT_ID is mapped to SWITCH_L2MC_MEMBER_ATTR_OUTPUT_HANDLE; it is intended to be used with SAI_OBJECT_TYPE_BRIDGE_PORT which has to be mapped to port or LAG or tunnel before sending it down to BMAI layer,
  - SAI_L2MC_GROUP_MEMBER_ATTR_L2MC_ENDPOINT_IP is mapped to dest_ip, but the value is ignored by BMAI under the assumption that it will always match the dst_ip of the p2p tunnel specified by the output_handle.

For the record IPMC-related SAI definitions now work as described below:
- saiipmc.h - is implemented in saiipmc.cpp where we handle CRUD operations for IPMC entries:
  - SAI_OBJECT_TYPE_IPMC_ENTRY is mapped to a relevant *ipmc_route* object,
  - SAI_IPMC_ENTRY_ATTR_OUTPUT_GROUP_ID is mapped to *group_handle*,
  - SAI_IPMC_ENTRY_ATTR_PACKET_ACTION is not supported (i.e. the FORWARD action is applied regardless of the value),
  - ipmc_entry struct attributes are mapped to a new *ipmc_route* object that programs IPMC forwarding paths;
    - vr_id (vrf ID) -> vrf_handle,
    - type (one of SAI_IPMC_ENTRY_TYPE_SG or SAI_IPMC_ENTRY_TYPE_XG) -> is used to determine whether we program src_ip or set it to all zeros,
    - destination (IP address) -> grp_ip,
    - source (IP address) -> src_ip (programmed only for **_multicast_route_s_g*),
- saiipmcgroup.h – is implemented in ipmcgroup.cpp where we create IP multicast groups with a set of members:
  - SAI_OBJECT_TYPE_IPMC_GROUP is mapped to *ipmc_group*
  - SAI_IPMC_GROUP_ATTR_IPMC_MEMBER_LIST is mapped to SWITCH_IPMC_GROUP_ATTR_IPMC_MEMBERS,
  - SAI_IPMC_GROUP_ATTR_IPMC_OUTPUT_COUNT which stores number of IPMC output in the group is not supported,
  - SAI_OBJECT_TYPE_IPMC_GROUP_MEMBER is mapped to *ipmc_member*,
  - SAI_IPMC_GROUP_MEMBER_ATTR_IPMC_GROUP_ID is mapped to SWITCH_IPMC_MEMBER_ATTR_IPMC_GROUP_HANDLE,
  - SAI_IPMC_GROUP_MEMBER_ATTR_IPMC_OUTPUT_ID is mapped to SWITCH_IPMC_MEMBER_ATTR_OUTPUT_HANDLE,
  - custom SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_L2MC_OUTPUT_ID is added as an extension SAI attribute and it is mapped to SWITCH_IPMC_MEMBER_ATTR_L2MC_GROUP_HANDLE,
  - custom SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_EGRESS_OBJECT is added as an extension SAI attribute and it is mapped to SWITCH_IPMC_MEMBER_ATTR_EGRESS_OBJECT_HANDLE.

___
\* From SAI documentation we read:

*L2 Multicast, a basic layer 2 feature, which provides a security solution by isolating flooding domain from VLAN to L2 multicast group. Meanwhile, it decreases the traffic load on a Layer 2 switch. L2 multicast can be implemented by multicast FDB lookup table (referred to as MAC based L2 multicast) or SG/\*G lookup table (referred to as IP based L2 multicast, known as snooping). This document covers the implementation of MAC based L2 multicast.*

According to customer requirements we assume that we need to support IP-based L2 multicast only.
___

### Reverse Path Forwarding support
Reverse-Path Forwarding (or RPF) is a technique that ensures loop-free forwarding of multicast packets in multicast routing. It is simply about specifying IPMC members that are allowed to be a source of multicast packets.

RPF has a connotation with Protocol-Independent Multicast (PIM) variants. In general there are:
- PIM Sparse Mode (PIM-SM) - when multicast routing is rooted in one selected node,
- PIM Dense Mode (PIM-DM) - when any node may be a source of multicast packets,
- Bidirectional PIM (Bidir-PIM) - when multicast tree looks like in PIM-SM but it differs from it with the fact that the tree is bi-directional.

In BMAI we only support PIM-SM what causes that RPF should limit the number of *ipmc_member*s, that are allowed to be a source of multicast packets, to one.

To support Layer 3 multicast RPF functionalities, SAI_OBJECT_TYPE_RPF_GROUP and SAI_OBJECT_TYPE_RPF_GROUP_MEMBER must be supported. They are implemented in sairpfgroup.cpp module. SAI RPF group may contain several members but it doesn't allow to specify PIM mode. In case a user will try to add more RPF group members, it will be forbidden by BMAI layer where it is guarded by *before_rpf_member_create()* trigger. Due to the fact that we only support PIM-SM in BMAI and in SAI we cannot set PIM mode for RPF group object, it is manually set to SWITCH_IPMC_ROUTE_ATTR_PIM_MODE_SM in sairpfgroup.cpp. Potentially, if we will support other variant of PIM in the future, we could determine the mode basing on number of RPF group members added.

## Code Changes

### Multicast-related user objects

***multicast_group*** has been split into *l2mc_group* and *ipmc_group* objects. These objects are equivalents of SAI_OBJECT_TYPE_IPMC_GROUP and SAI_OBJECT_TYPE_L2MC_GROUP. Both of them trigger creation of *mc_mgid* object. *type* attribute is not used anymore. Both objects store a list of multicast group members belonging to given multicast group.

***multicast_member*** has been split into separate objects for L2MC and IPMC handling. There are new *l2mc_member* and *ipmc_member* objects. Each of them may be related to one and only one *l2mc*- or *ipmc_group* that is stored in *l2mc*- or *ipmc_group_handle* attribute respectively. l2mc_member supports port, lag, and tunnel as types of objects stored in the output_handle attribute. ipmc_member supports rif as the type of object stored in the output_handle attribute. Objects of type tunnel require special handling since they require their own PRE nodes to be programmed with RID values specific to the tunnel. In order to drive this programming, internal user objects l2mc_member_tunnel and ipmc_member_vlan_tunnel are also introduced, that are programmed in addition to the l2mc_member and ipmc_member objects.

There is also a customer-specific request to support additional SAI attributes: SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_L2MC_OUTPUT_ID and SAI_IPMC_GROUP_MEMBER_ATTR_EXTENSIONS_EGRESS_OBJECT. To handle them, two additional *ipmc_member*'s attributes: *l2mc_group_handle* and *egress_object_handle* was added.

***multicast_route*** is splitted into *l2mc_bridge* and *ipmc_route*. They are intended to configure "route" and "bridge" entries responsible for multicast packets forwarding.

*l2mc_bridge* has following attributes:
- *src_ip* - stores src IP address for (S, G) multicast entries,
- *grp_ip* - stores multicast group IP address,
- *vlan_handle* - stores VLAN handle,
- *group_handle* - stores a handle of destination *l2mc_group*.

*ipmc_route* has following attributes:
- *src_ip* - stores src IP address for (S, G) multicast entries,
- *grp_ip* - stores multicast group IP address,
- *vrf_handle* - stores VRF handle,
- *pim_mode* - allowing to set PIM variant,
- *group_handle* - stores a handle of destination *ipmc_group*,
- *rpf_group_handle* - stores a handle of Reverse Path Forwarding group.

Note that RPF group and PIM modes don't apply to L2MC so they are not considered as *l2mc_bridge* attributes.

### Multicast-related auto objects

***mc_mgid*** class was updated to create pre.mgid table entries for each *l2mc_bridge* and *ipmc_group* next to previously supported *vlan*s. The reason why in case of L2MC we use bridge object instead of group object is that we can have multiple bridges using the same *l2mc_group* as a destination L2MC multicast group handle, which requires separate pre.mgid and pre.nodes since the RID values may be different for each *l2mc_bridge*. In case of IPMC, pre.mgid and pre.nodes can be shared by multiple *ipmc_routes* using the same *ipmc_group*, since each *ipmc_member* maps to a specific VLAN, allowing reuse of the same RID.

***mc_node*** class has been split into *l2mc_node*, *mc_node_tunnel*, *ipmc_node*.
- *l2mc_node* programs one pre.node entry per each *l2mc_bridge* (as the node is practically needed only when bridge to *l2mc_group* exists). It stores a list of ports (or LAGs) on many-to-one basis. They are programmed as *l2mc_member*s,
- *mc_node_tunnel* :
  - For each *l2mc_member* of type tunnel, in case of L2MC a separate *mc_node_tunnel* auto object programs one pre.node for each *l2mc_bridge* using the corresponding l2mc_group.
  - For each *l2mc_member* of type tunnel, in case of IPMC a separate *mc_node_tunnel* auto object programs one pre.node for each *ipmc_member* specifying the corresponding l2mc_group.
  - For each *vlan_member* of type tunnel, in case of IPMC a separate *mc_node_tunnel* auto object programs one pre.node for each *ipmc_member* (that does not specify l2mc_group) specifying a RIF on the corresponding VLAN.
- *ipmc_node* programs one pre.node entry per each *ipmc_member* created for an object of type RIF. Each of them stores port(s) (or LAGs) corresponding to that RIF. They are programmed as *ipmc_member*s.

In case of L2MC, one Level1 node is created for the whole group to handle ports and LAGs and for each tunnel belonging to the group there is separate additional node. Each *l2mc_member* created for port or lag object is considered as separate Level2 node connected to the common node. Each member created for tunnel object is mapped 1:1 to its own node.

Relations between PRE entries can be pictured as below:

```
                                    +--------+
                                    |  mgid  |  (per l2mc_bridge)
                                    +--------+
                                   / .. | ..  \
                           +------+     +-+    +----------+
                          /                \              |
                     +--------+         +--------+    +--------+
   (per l2mc_bridge) |  node  |         | node_x | .. | node_z |  (per tunnel
                     +--------+         +--------+    +--------+  l2mc_member)
                     /   |    \              \             |
                    /    |  ..  \          +-----+      +-----+
     (per      +-----+ +-----+    +-----+  | p_x |  ..  | p_z |
   non-tunnel  | p_0 | | p_1 | .. | p_n |  +-----+      +-----+
  l2mc_member) +-----+ +-----+    +-----+
```
*Fig2: PRE programming for L2MC*

In case of IPMC, Level1 node represents individual network (separate broadcast domain). It may be created with an object of type RIF or tunnel. When a RIF is of type port or subport, there is one Level2 node per each Level1 node. When the RIF is of type VLAN, there is separate Level2 node for each port and LAG that is a member of the VLAN and also there is separate Level1 node with corresponding Level2 node for each tunnel being a member of the *l2mc_group* (if specified for the *ipmc_member*) or a member of the VLAN (if no *l2mc_group* is specified for the *ipmc_member*).

```
                                    +-----------+
                                    |   mgid    |  (per ipmc_group)
                                    +-----------+
                                   /  ..  |  ..  \
(per VLAN RIF  +------------------+       |       \
 ipmc_member) /                       +--------+    +--------+
       +--------+         (per tunnel | node_x | .. | node_z | (per port/subport
       | node_a |        VLAN member) +--------+    +--------+  RIF ipmc_member)
       +--------+                          |             |
        / .. |                          +-----+       +-----+
  +------+ +------+                     | p_x |   ..  | p_z |
  | p_a0 | | p_an | (per non-tunnel     +-----+       +-----+
  +------+ +------+ VLAN members)
```
*Fig3: PRE programming for IPMC*

***multicast_factory*** was responsible for choosing the right type of route or bridge depending on settings passed by a user. It has been split into *multicast_bridge_factory* and *multicast_route_factory* which handle choosing of bridges or routes separately.

***rid_table*** programs entries of P4 *rid* table that determines egress BD and/or nexthops (in case the output of multicast traffic is a tunnel). Previously, there were separate tables for broadcast/multicast- (*rid* table in multicast.p4 that set only *bd*) and tunnel-related purposes(*tunnel_rid* in tunnel.p4 that set *nexthop* and *tunnel_nexthop*). Both tables were joined in one *rid* table defined in multicast.p4 module. When egress Replication ID is set, the table sets one of following actions:
- *rid_miss* - when the *rid* was not found in entries keys,
- *rid_hit_mc* - which sets (egress) *bd* for L2 replication and L3 replication with local ports only,
- *rid_hit_tunnel* - which sets *nexthop* and *tunnel_nexthop* for L2 replication and flooding to a tunnel,
- *rid_hit_tunnel_mc* - which sets *bd*, *nexthop*, and *tunnel_nexthop* for L3 replication to a tunnel.

The table is programmed by *rid_table* class, that programs an entry for each of following objects:
- *vlan* - *rid_hit_mc* action,
- *tunnel_replication_resolution* - *rid_hit_tunnel* action,
- *rif* of type port or subport that is specified by at least one *ipmc_member* - *rid_hit_mc* action - this is a little bit tricky because for IPMC members with *output_handle* of type RIF we have to program Replication ID being equal to BD value that is used for given RIF. Due to this reason we cannot assign RIDs in *ipmc_member*-based manner so in *compute_rid()* utils function we retrieve RIF's BD and in *rid_table* we program the action basing on it. *rid* table entries are required only for those RIFs that are used by any IPMC members thus we check this condition whenever *rid_table* constructor is called with parent *rif*. In order to handle creation and deletion of such rid table entries only when necessary, the constructor is called with parent ipmc_member, in which case we update the rid_table object corresponding to that ipmc_member's rif.
- *ipmc_member_vlan_tunnel* - *rid_hit_tunnel_mc* action.
___
As consequences of the changes described above, we have the following objects defined in BMAI objects model:
- User objects:
  - *l2mc_group* - Layer 2 Multicast group,
  - *l2mc_member* - Layer 2 Multicast member - object of type *port*, *lag*, or *tunnel*,
  - *l2mc_member_tunnel* - internal object where for every *l2mc_member* with output handle of type tunnel, a separate instance is created for each *l2mc_bridge* using the corresponding *l2mc_group*; triggers *mc_node_tunnel*,
  - *l2mc_bridge* - forwarding path for Layer 2 Multicast traffic; triggers *mc_mgid*, *l2mc_node*, and *multicast_bridge_factory*,
  - *ipmc_group* - IP Multicast group; triggers *mc_mgid*,
  - *ipmc_member* - IP Multicast member - object of type *rif* or *tunnel*; triggers *ipmc_node*,
  - *ipmc_member_vlan_tunnel* - internal object where for every l2mc_member with output handle of type tunnel or for every vlan_member with output handle of type tunnel, a separate instance is created for each ipmc_member that specifies a rif using that l2mc_group or vlan, respectively; triggers *mc_node_tunnel*,
  - *ipmc_route* - forwarding path for IP Multicast traffic; triggers *ipmc_route_factory*,
- Auto objects (and corresponding classes):
  - *rid_table* - programs entries in P4 *rid* table which sets bd- and nexthop-related values for multicast and L2 VxLAN flooding functionalities,
  - *mc_mgid* - represents multicast group programmed inside fixed PRE mgid table. It is created for each *vlan* (for flooding purposes), each *l2mc_bridge*, and each *ipmc_group* object created by user,
  - *mc_node_vlan* - creates Level1 nodes corresponding to *vlan* objects,
  - *mc_node_vlan_member* - creates separate Level1 node for each *vlan_member* of the type tunnel, required for L2 VxLAN flooding functionalities,
  - *l2mc_node* - creates Level1 nodes for all ports and LAGs belonging to the same *l2mc_group* (with separate Level2 nodes for each of these ports and LAGs),
  - *mc_node_tunnel* - creates Level1 nodes for each *l2mc_member_tunnel* and for each *ipmc_member_vlan_tunnel*; each of such nodes has only one corresponding Level2 node determined from nexthop resolution of the flood_nexthop_handle of tunnel_replication_resolution,
  - *ipmc_node* - creates Level1 nodes for RIFs, each of such nodes has single Level2 node if the type of the RIF is port or sub-port or several Level2 nodes if the type of the RIF is VLAN (then the node is created for each *vlan_member* unless there is SWITCH_IPMC_MEMBER_ATTR_L2MC_GROUP_HANDLE attribute specified - in this case there are nodes for particular members of given *l2mc_group*),
  - *multicast_bridge_factory* - chooses one of *mulitcast_bridge* objects to program correct P4 table according to bridge parameters passed by user while creating *l2mc_bridge*,
  - *multicast_route_factory* - chooses one of *mulitcast_route* objects to program correct P4 table according to route parameters passed by user while creating *ipmc_route*,
  - *[ipv4_/ipv6_]multicast[_route/_bridge][_s_g/_x_g]* - the accurate bridge/route objects that program correct P4 table that determine multicast packets forwarding paths.

### *l2mc_member_tunnel* object handling
*l2mc_member_tunnel* is an internal user object created for every *l2mc_bridge* and every *l2mc_member* with output handle of type tunnel, that belongs to an *l2mc_group* used by the bridge, as it is shown in Fig4:
```
                        +-------------+
                        | l2mc_bridge |
                        +-------------+
                              \ /     \
                               |       \
                        +------------+  \
                        | l2mc_group |   \
                        +------------+    +--------------------+
                              ||          | l2mc_member_tunnel |
                              /\          +--------------------+
                        +-------------+   /         #
                        | l2mc_member |  /          #
                        +-------------+ /   +----------------+
                               |       /    | mc_node_tunnel |
                          +--------+  /     +----------------+
                          | tunnel |-+
                          +--------+

  legend:        = or ||            membership
                 - or |             object-attribute relation
          -< or >- or \ /  or  |    one-to-many relation
                       |      / \
                    #               parent-child relation
```
*Fig4: Schema of objects related to l2mc_member_tunnel object handling*

Although it is a user object, it is created automatically every time when:
- *l2mc_bridge* is created and in corresponding destination *l2mc_group* there is at least one member of type *tunnel*; one instance of *l2mc_member_tunnel* is created for each such *l2mc_member*,
- *l2mc_member* is created and there is an existing *l2mc_bridge* object that uses its *l2mc_group* as a destination; one instance of *l2mc_member_tunnel* is created for each such *l2mc_bridge*.

Analogically, the instances of *l2mc_member_tunnel* are removed each time when some *l2mc_bridge* or *l2mc_member* of type *tunnel* is removed. These operations are performed in: after_l2mc_bridge_create(), before_l2mc_bridge_delete(), after_l2mc_member_create(), and before_l2mc_member_delete() functions that are registered as appropriate triggers related to SWITCH_OBJECT_TYPE_L2MC_BRIDGE and SWITCH_OBJECT_TYPE_L2MC_MEMBER.

*l2mc_member_tunnel* object has two attributes that allow to find a relevant instance of the object while *mc_node_tunnel* is being programmed or when the object itself is being used. The attributes store a handle of a tunnel (in *SWITCH_L2MC_MEMBER_TUNNEL_ATTR_TUNNEL_HANDLE*) corresponding to the *l2mc_member* and a handle of a bridge (in *SWITCH_L2MC_MEMBER_TUNNEL_ATTR_L2MC_BRIDGE_HANDLE*) created for an *l2mc_group* containing the tunnel.

### *ipmc_member_vlan_tunnel* object handling
*ipmc_member_vlan_tunnel* is an internal user object created for every tunnel member of VLAN that is an SVI and every *ipmc_member* that uses the RIF, as it is shown in Fig5 and Fig6 below:
```
                        +-------------+
                        | ipmc_member |
                        +-------------+
                               |      \\
                            +-----+    \\
                            | rif |     \\
                            +-----+      \\
                               |          \\
   [there may be many ]     +------+       +-------------------------+
   [many more members ]=====| vlan |       | ipmc_member_vlan_tunnel |
   [with ports or LAGs]     +------+       +-------------------------+
                               ||           /           #
                               /\          /            #
                        +-------------+   /    +----------------+
                        | vlan_member |  /     | mc_node_tunnel |
                        +-------------+ /      +----------------+
                               |       /
                          +--------+  /
                          | tunnel |-+
                          +--------+

  legend:        = or ||            membership
                 - or |             object-attribute relation
          -< or >- or \ /  or  |    one-to-many relation
                       |      / \
                    #               parent-child relation
```
*Fig5: Schema of objects related to ipmc_member_vlan_tunnel object handling when no l2mc_group is specified for the ipmc_member*

```
              +------------+                      +-------------+
              | l2mc_group |----------------------| ipmc_member |
              +------------+                      +-------------+
              //                                   /    ||
             //                 +-----+           /     ||
            //                  | rif |----------+      ||
  ++=======++                   +-----+                 ||
  ||                               |                    ||
  ||  [there may be many ]     +------+       +-------------------------+
  ||  [many more members ]=====| vlan |       | ipmc_member_vlan_tunnel |
  ||  [with ports or LAGs]     +------+       +-------------------------+
  ||                              ||            /          #
  ||                              /\           /           #
  ||                        +-------------+   /    +----------------+
  ||                        | vlan_member |  /     | mc_node_tunnel |
  ||                        +-------------+ /      +----------------+
  ||                              |        /
  ||      +-------------+     +--------+  /
  ++======| l2mc_member |-----| tunnel |-+
          +-------------+     +--------+

  legend:        = or ||            membership
                 - or |             object-attribute relation
          -< or >- or \ /  or  |    one-to-many relation
                       |      / \
                    #               parent-child relation
```
*Fig6: Schema of objects related to ipmc_member_vlan_tunnel object handling when l2mc_group is specified for the ipmc_member*

Although it is a user object, it is created automatically every time when:
- *ipmc_member* with *output_handle* of type RIF of type VLAN is being created and there is at least one *tunnel* that is a member of the VLAN - then one instance is created for each of such tunnels,
- a tunnel is being added to a VLAN being an SVI that is an *output_handle* of existing *ipmc_member*.

Analogically, the instances of *ipmc_member_vlan_tunnel* are removed each time when mentioned *ipmc_member* is removed (then all objects corresponding to the *ipmc_member* are deleted) or when the tunnel is being removed from the VLAN being an SVI that is an *output_handle* of existing *ipmc_member*.

*ipmc_member_vlan_tunnel* object has two attributes that allow to program pre.node entry via *mc_node_tunnel* class and to find it's entries that correspond to given *ipmc_member*. The attributes store a handle of a tunnel (in *SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_TUNNEL_HANDLE*) and a handle of an *ipmc_member* (in *SWITCH_IPMC_MEMBER_VLAN_TUNNEL_ATTR_IPMC_MEMBER_HANDLE*). Additionally, the object has *rid* attribute that stores Replication ID assigned for it. The object also participate in membership of *vlan_tunnel_members* attribute of *ipmc_member* object.

*ipmc_member_vlan_tunnel* instances are created and deleted by the *evaluate_tunnel_members()* function that is called upon create/update/delete of an ipmc_node whose parent's *output_handle* specifies a RIF of type VLAN.

### RID and node indexes computation
Past implementation made use of simple utils functions to compute Replication IDs and pre.node indexes for multicast, flooding, and broadcasting purposes. These functions simply returned a unique number basing on object ID passed as an argument. To guarantee uniqueness of the returned values the functions used bit shifting operations on the OIDs. Number of shifted bits was dependent on a number of different object types that were supported by the functions. As the number increased, this simple solution would lead to significant waste of identifiers space and uniqueness couldn't be guaranteed anymore.

To solve this issue another approach was used instead. RID and node indexes computation is partially based on idAllocators.
1. Each type of pre.node-programming objects stores its index as an attribute. New idAllocator called *pre_node_manager* is declared in API's utils along with appropriate allocate and release functions. The value is determined and allocated once during object creation while during removal it is released. Wherever node's index value is needed for P4 entries programming they are read from relevant node's attribute.
To ensure correct pre.mgid entries programming, all the objects that store index value for particular nodes must exist before *mc_mgid* is programmed. In cases when *mc_mgid* is triggered by the same parents that trigger nodes-related object, the order of objects creation is managed by assigning a priority to the auto object. This applies to *mc_node_vlan*, *mc_node_vlan_member*, *l2mc_node*, *ipmc_node*.
2. For each type of objects that requires Replication ID (for both: multicast or flooding purposes) we determine unique RID value using reworked *compute_rid()* function. Having in mind that **in P4 layer we assumed that RID is equal to bd and for VLAN flooding bd is always equal to *vlan_id*** in *compute_rid()* first 8k numbers are reserved for *mc_node_vlan* objects (for which we simply return *vlan_id* of given *vlan* object) and for *ipmc_node* created for RIFs. Next to this for all other object types new idAllocators called *rid_manager* is declared in API's utils along with appropriate allocate and release functions. They preserve the assumption about reserved bd-related values. Currently, *compute_rid()* may be called from any place and it supports objects of types:
- *vlan* - for flooding within VLANs and L2MC functionalities; stored in *rid* attribute of *mc_node_vlan* or *l2mc_node* to be easily accessible from *mc_mgid* class,
- *rif* - for multicasting packets to Level 1 nodes corresponding to RIF objects,
- *tunnel_replication_resolution* - for L2 VxLAN flooding and L2 multicast,
- *ipmc_member_vlan_tunnel* - for ipmc to tunnels as *vlan_member*s or *l2mc_member*s.

____
## To Do / WIP list
- [x] Rework multicast-related user objects: [SWI-5644](https://jira.devtools.intel.com/browse/SWI-5644), [PR 3325](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3325)
- [x] Rework multicast-related auto objects for L2MC: [SWI-5655](https://jira.devtools.intel.com/browse/SWI-5655), [PR 3429](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3429)
- [x] Rework multicast-related auto objects for non-tunnel IPMC: [SWI-5759](https://jira.devtools.intel.com/browse/SWI-5759), [PR 3537](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3537/)
- [x] Rework multicast-related objects schema for IPMC group with VLAN RIF containing tunnel: [SWI-6004](https://jira.devtools.intel.com/browse/SWI-6004), [PR 3740](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3740)
- [ ] Rework multicast-related objects schema for IPMC group with member of type tunnel
- [x] Implement API PTF tests for L2MC - added in [PR 3429](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3429)
- [x] Implement API PTF tests for non-tunnel IPMC - added in [PR 3537](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3537/)
- [x] Implement SAI support for L2MC: [SWI-5398](https://jira.devtools.intel.com/browse/SWI-5395), [PR 3473](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3473)
- [x] Implement SAI support for IPMC: [SWI-5394](https://jira.devtools.intel.com/browse/SWI-5394), [PR 3570](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3570)
- [x] Implement SAI PTF tests for L2MC: [SWI-5623](https://jira.devtools.intel.com/browse/SWI-5623), [PR 3473](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3473)
- [x] Implement SAI PTF tests for IPMC: [SWI-5623](https://jira.devtools.intel.com/browse/SWI-5623), [PR 3570](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3570)
- [x] Implement SAI PTF tests for IPMC group with VLAN RIF containing tunnel - added in [PR 3740](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3740)
- [ ] Implement SAI PTF tests for IPMC group with member of type tunnel
- [x] Fix L2MC issue with multiple L2MC bridges using the same L2MC group with at least member being an object of type tunnel: [SWI-6005](https://jira.devtools.intel.com/browse/SWI-6005), [PR 3630](https://github.com/intel-restricted/networking.switching.barefoot.bf-switch/pull/3630)
