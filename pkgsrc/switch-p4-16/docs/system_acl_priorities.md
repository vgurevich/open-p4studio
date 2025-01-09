# System ACL priority management
This document describes how priorities are managed for entries within Ingress system ACl table.

## P4 code
*pipe.ingress_system_acl* is the HW table used to manage packet actions within switch.p4. The packet action is derived based on matching various PHV fields previously set in the pipeline.

## Default Entries
The *default_ingress_system_acl* class adds a bunch of entries by default. These are required to perform basic packet handling for most common cases. These entries are added with priority starting at 32870 and upwards.

### List of entries
The class has priority starting at 32870 and every entry has the priority increased by one. (Tofino has higher priority for lower values).

The entries are ordered as L2, L3 and finally ACL and QoS based drops.

| Entry                 | Action     | Comments
|-----------------------|------------|---------
| L2 drop reasons       | drop       | match on local_md.l2_drop_reason
| same mac check        | drop       |
| pv miss               | drop       |
| stp blocked           | drop       | No learning
| stp learn             | drop       | drop and learn
| L2 self fwd check     | drop       | !routed, iport == eport
| dmac reserved         | drop       |
| l3 drop reasons       | drop       |
| ipv4 unicast disabled | drop       | 
| ipv6 unicast disabled | drop       | 
| ipmc dmac mismatch    | drop       |
| blackhole route       | drop       |
| ip options            |            | drop/trap/copy based on vrf properties
| rmac hit non ip       | drop       |
| routed, ttl=1, cpu    | permit     | ingress port is CPU
| routed, ttl=1         | trap       |
| ip src link local     | drop       |
| ip dst link local     | drop       |
| ip6 src link local    | permit     | ingress port is cpu
| ip6 src link local    | trap       | routed packet
| my ip                 | trap       |
| my ip subnet          | trap       |
| glean                 | trap       |
| dip loopback          | drop       |
| mcast scope 0         | drop       |
| mcast scope 1         | drop       |
| dip unspecified       | drop       |
| sip unspecified       | drop       |
| lpm4 miss             | drop       |
| lpm6 miss             | drop       |
| l3 self fwd check     | permit     |
| sip bcast             | drop       |
| sip class e           | drop       |
| mpls                  | drop       |
| mpls state            |            | drop/trap/copy based on mpls trap state
| srv6                  |            | drop/trap/copy based on srv6 trap state
| igmp non routable     | drop       |
| unknown l3 mcast      | drop       |
| L3 port, rmac miss    | drop       |
| uc dip, mc mac        | drop       |
| acl deny              | drop       |
| racl deny             | drop       |
| port meter            | drop       |
| storm control         | drop       |
| pfc_wd                | drop       |

## Hostif Trap Entries
The hostif trap entries are user added entries usually coming from applications/NOS. These entries are limited to priority range 1 - 16384. SONiC has priorities inverted. They are mapped to this range by SAI code.

Please refer to list of hostif_traps supported in schema/appObj/hostif.json.

| Trap type           | Match Fields                             | Comments
|---------------------|------------------------------------------|---------
| **L2 traps**        |                                          |
| STP                 | dmac=01:80:C2:00:00:00 |
| LACP                | dmac=01:80:C2:00:00:02 |
| EAPOL               | etype=0x888E |
| LLDP                | dmac=01:80:C2:00:00:0E, etype=0x88CC |
|                     | dmac=01:80:C2:00:00:03, etype=0x88CC |
| PVRST               | dmac=01:00:0C:CC:CC:CD |
| IGMP_TYPE_QUERY     | ip_proto=2, sport=0x11, igmp_snooping=1 |
| IGMP_TYPE_LEAVE     | ip_proto=2, sport=0x17, igmp_snooping=1 |
| IGMP_TYPE_V1_REPORT | ip_proto=2, sport=0x12, igmp_snooping=1 |
| IGMP_TYPE_V2_REPORT | ip_proto=2, sport=0x16, igmp_snooping=1 |
| IGMP_TYPE_V3_REPORT | ip_proto=2, sport=0x22, igmp_snooping=1 |
| SAMPLEPACKET        | local_md.flags.sample_packet=true |
| UDLD                | dmac=01:00:0C:CC:CC:CC |
| PTP                 | ip_proto=17, dport=319,320, etype=0x88F7 |
| DHCP_L2             | ip_proto=17, sport=67, dport=68, ip_type=v4 |
|                     | ip_proto=17, sport=68, dport=67, ip_type=v4 |
| DHCPV6_L2           | ip_proto=17, sport=546, dport=547, ip_type=v6 |
|                     | ip_proto=17, sport=547, dport=546, ip_type=v6 |
| **L3 traps**        | *v4 or v6 uc/mc enable or myip, pv_miss=false* |
| ARP_REQUEST         | pkt_type=bc, etype=0x806, arp_code=1, v4_uc_en=1, pv_miss=false, (stp=forwarding) |
|                     | pkt_type=uc, etype=0x806, arp_code=1, v4_uc_en=1, pv_miss=false, rmac_hit=1 |
| ARP_RESPONSE        | pkt_type=bc, etype=0x806, arp_code=2, v4_uc_en=1, pv_miss=false, (stp=forwarding) |
|                     | pkt_type=uc, etype=0x806, arp_code=2, v4_uc_en=1, pv_miss=false, rmac_hit=1 |
| ISIS                | dmac=09:00:2B:00:00:05, pv_miss=false, stp=forwarding |
|                     | dmac=01:80:C2:00:00:14, pv_miss=false, stp=forwarding |
|                     | dmac=01:80:C2:00:00:15, pv_miss=false, stp=forwarding |
| DHCP                | ip_proto=17, sport=67, dport=68, myip=1, ip_type=v4, pv_miss=false, stp=forwarding |
|                     | ip_proto=17, sport=67, dport=68, dip=0xFFFFFFFF, ip_type=v4, pv_miss=false, stp=forwarding |
|                     | ip_proto=17, dport=67, myip=1, ip_type=v4, pv_miss=false, stp=forwarding |
|                     | ip_proto=17, dport=67, dip=0xFFFFFFFF, ip_type=v4, pv_miss=false, stp=forwarding |
| OSPF                | ip_proto=86, v4_uc_en=1, dip=0xE0000005, ip_type=v4, pv_miss=false, stp=forwarding |
|                     | ip_proto=86, v4_uc_en=1, dip=0xE0000006, ip_type=v4, pv_miss=false, stp=forwarding |
| PIM                 | ip_proto=103, v4_uc_en=1, pv_miss=false, stp=forwarding |
| VRRP                | ip_proto=112, v4_uc_en=1, ip_type=v4, dip=0xE0000012, pv_miss=false, stp=forwarding |
| DHCPV6              | ip_proto=17, v6_uc_en=1, dip=FF02::1:2, ip_type=v6, sport=546, dport=547, pv_miss=false, stp=forwarding |
|                     | ip_proto=17, ip_type=v6, sport=546, dport=547, myip=1, pv_miss=false, stp=forwarding |
|                     | ip_proto=17, ip_type=v6, sport=547, dport=546, myip=1, pv_miss=false, stp=forwarding |
| OSPFV6              | ip_proto=86, v6_uc_en=1, dipFF02::5, ip_type=v6, pv_miss=false, stp=forwarding |
|                     | ip_proto=86, v6_uc_en=1, dipFF02::6, ip_type=v6, pv_miss=false, stp=forwarding |
| VRRPV6              | ip_proto=112, v6_uc_en=1, ip_type=v6, dip=0xFF02::12, pv_miss=false, stp=forwarding |
| IPV6_ND             | ip_proto=58, v6_uc_en=1, etype=0x86dd, pv_miss=false, sport=133-137, pv_miss=false, stp=forwarding |
| IPV6_MLD_V1_V2      | ip_proto=58, mld_snooping=true, sport=130, pv_miss=false, stp=forwarding |
| IPV6_MLD_V1_REPORT  | ip_proto=58, mld_snooping=true, sport=131, pv_miss=false, stp=forwarding |
| IPV6_MLD_V1_DONE    | ip_proto=58, mld_snooping=true, sport=132, pv_miss=false, stp=forwarding |
| MLD_V2_REPORT       | ip_proto=58, mld_snooping=true, sport=143, pv_miss=false, stp=forwarding |
| SNAT_MISS           | local_md.nat_hit=src_none, local_md.same_zone=1 |
| DNAT_MISS           | local_md.nat_hit=dst_none |
| **Local traps**     | *myip=true, pv_miss=false for all* |
| MYIP                | v4_uc_en=1, myip=true, pv_miss=false |
|                     | v6_uc_en=1, myip=true, pv_miss=false |
| SSH                 | ip_proto=6, dport=22, myip=true, pv_miss=false |
| SNMP                | ip_proto=17, dport=161, myip=true, pv_miss=false |
| BGP                 | ip_proto=6, etype=0x800, dport=179, myip=true, v4_uc_en=1, pv_miss=false |
|                     | ip_proto=6, etype=0x800, sport=179, myip=true, v4_uc_en=1, pv_miss=false |
| BGPV6               | ip_proto=6, etype=0x800, dport=179, myip=true, v6_uc_en=1, pv_miss=false |
|                     | ip_proto=6, etype=0x800, sport=179, myip=true, v6_uc_en=1, pv_miss=false |
| BFD                 | ip_proto=17, dport=3784, ip_type=v4, myip=true, pv_miss=false |
|                     | ip_proto=17, dport=4784, ip_type=v4, myip=true, pv_miss=false |
| BFDV6               | ip_proto=17, dport=3784, ip_type=v6, myip=true, pv_miss=false |
|                     | ip_proto=17, dport=4784, ip_type=v6, myip=true, pv_miss=false |
| LDP                 | ip_proto=6, etype=0x800, sport=179, v4_uc_en=1, myip=true, pv_miss=false |
|                     | ip_proto=6, etype=0x800, dport=179, v4_uc_en=1, myip=true, pv_miss=false |
|                     | ip_proto=17, etype=0x800, dport=179, dip=224.0.0.2, pv_miss=false |
| GNMI                | ip_proto=6, dport=9339, myip=true, pv_miss=false |
| P4RT                | ip_proto=6, dport=9559, myip=true, pv_miss=false |
| NTPCLIENT           | ip_proto=(6, 17), sport=123, myip=true, pv_miss=false |
| NTPSERVER           | ip_proto=(6, 17), dport=123, myip=true, pv_miss=false |
| **Exceptions**      |                                          |
| L3_MTU_ERROR        | local_md.checks.mtu=0 |
| TTL_ERROR           | ttl=1, routed=true, mpls, vrf violation |
| MPLS_ROUTER_ALERT   | hdr.mpls.valid=true, local_md.mpls_router_alert=true, routed=true |
| MPLS_TTL_ERROR      | hdr.mpls.valid=true, hdr.mpls.ttl=0,1, routed=true |
| **Custom**          |                                          |
| ARP_SUPPRESS        | pkt_type=bc, etype=0x806, arp_code=1, v4_uc_en=1, pv_miss=false, arp_suppress=true |
| ND_SUPPRESS         | ip_proto=58, v6_uc_en=1, etype=0x86dd, pv_miss=false, sport=133-137, arp_suppress=true |
| ICMP                | ip_proto=1, v4_uc_en=1, etype=0x800, pv_miss=false |
| ICMPV6              | ip_proto=58, v6_uc_en=1, etype=0x86dd, pv_miss=false |
| ICCP                | ip_proto=6, etype=0x800, dport=8888, v4_uc_en=1, myip=1, pv_miss=false |
|                     | ip_proto=6, etype=0x86dd, dport=8888, v6_uc_en=1, myip=1, pv_miss=false |
