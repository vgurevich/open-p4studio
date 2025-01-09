# RMAC processing
This document describes the router MAC processing in switch.p4 and the accompanying control plane objects

## Revision
| Rev  | Rev Date   | Author(s)          | Change Description |
|------|------------|--------------------|--------------------|
| v0.1 | 09/03/2021 | Ravi Vantipalli    | Initial version    |

## P4 code
The majority of the P4 code is at shared/rmac.p4 in the IngressRmac control block.
There are 2 tables implementing the router RMAC check.
- *pv_rmac* for L3 RIF and switch RMAC entries.
- *vlan_rmac* for SVI RIF

1 table for inner rmac
- *rmac* for vxlan inner header

These tables are applied in the ingress_port_mapping control block right after PV tables.

### pv_rmac
This table is programmed for 4 cases
| Category            | Entry |
|---------------------|--------|
| L3 RIF (port)       | (port, *, rmac) |
| L3 RIF (sub_port)   | (port, vlan, rmac) |
| Switch              | (*, * , rmac)   |
| vlan_member (untagged) | (port, *, rmac) |

### vlan_rmac
This table has 1 entry for RIF of type vlan (vlan, rmac)

### Inner rmac
An inner RMAC table is also present for VXLAN related inner header processing. This table is in shared/tunnel.p4 simply called *rmac*. It is only applied when VXLAN is enabled in the profile.


## BMAI
The control plane implementation of router MAC checks involve 3 user objects and a bunch of associated asic objects.

| Object type | Attribute | Description |
|-------------|-----------|-------------|
| rif         | src_mac   | This acts as both ingress router MAC check and source of rewrite for outer ethernet source MAC address after routing |
| vrf         | src_mac   | This is used only for rewrite for inner Ethernet headers for tunnel cases |
| switch      | default_mac | A global ingress router MAC check and default source MAC for some tunnel scenarios |

The user to asic objects is setup as following

### RIF
- L3 RIF -> ingress_pv_rmac_for_rif
  - This class is used only when RIF type is PORT or SUB_PORT
- SVI -> ingress_vlan_rmac
  - This class is used only when RIF type is VLAN

Both of the above class have dependency on the SRC_MAC attribute of the RIF for update scenarios.

### SVI
- Port -> ingress_pv_rmac_for_port

This is the most complex of all the asic objects. The reason for this is it has to be updated for every untagged vlan_member added to a vlan if this vlan is an SVI. The following cases need to be handled
- SVI is created before vlan_member is created
- SVI is deleted before vlan_member is deleted
- vlan_member is created before SVI
- vlan_member is deleted before SVI

For this reason, a default entry is always created with rmac_miss action for every port creation.
 - (port, *, *)

When PORT_VLAN_ID attribute of the port is updated, this object is again evaluated. The entry is updated based on the presence of an SVI
- (port, *, *) - SVI not found for the vlan ID, action rmac_miss
- (port, *, rmac) - SVI found, action rmac_hit

Similarly, when a SVI is created on a vlan ID and this vlan has untagged vlan members, they have to be evaluated again using before/after triggers to update the entries.

### Switch
- device -> device_rmac

This is updated every time DEFAULT_MAC attribute of device is updated

### VXLAN
 - tunnel_mapper_entry -> vxlan_rmac

This class is created for tunnel_map_entry of type VNI_TO_VRF. The router MAC comes from the vrf object.

## RMAC user object
**TBD** when this object becomes equivalent of SAI mymac object
