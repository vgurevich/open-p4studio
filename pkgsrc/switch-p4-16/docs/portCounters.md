# Port Counters
This document contains counters related information for all ports in the switch.

## RMON Counters

Remote Montinoring (RMON) Counters displays information regarding packet sizes and physical layer errors per port in the switch hardware.

Counter descriptions are shown in below table.

Counter Name	         |   Description                          |
-------------------------|----------------------------------------|
 "IN_GOOD_OCTETS"        |  Good octets received
 "IN_ALL_OCTETS"         |  Octets received, including bad packets.
 "IN_GOOD_PKTS"          |  Number of frames received without error.
 "IN_ALL_PKTS"           |  Number of frames received overall (Good/Bad Frames)
 "IN_VLAN_PKTS"          |  Number of frames received which have VLAN (good frames)
 "IN_UCAST_PKTS"         |  Number of frames received with Unicast Address
 "IN_MCAST_PKTS"         |  Number of frames received with Multicast Address
 "IN_BCAST_PKTS"         |  Number of frames received with Broadcast Address
 "IN_NON_UCAST_PKTS"     |  Number of frames received with Multicast and Broadcast Address
 "IN_FCS_ERRORS"         |  Number of frames received with a bad frame check sequence
 "IN_ERROR_PKTS"         |  Number of frames received with any errors
 "IN_CRC_ERRORS"         |  Number of frames received whose CRC was stomped
 "IN_BUFFER_FULL"        |  Number of frames that were dropped from APP_FIFO because of Buffer Full condition
 "IN_FRAGMENTS"          |  Number of frames received which are runt (less than MinFrameSize) and have CRC error.
 "IN_JABBERS"            |  Number of frames received which exceed the programmed Jabber size and have CRC Error
 "IN_OCTETS_RATE"        |  Number of bytes received in the port rate buffer
 "OUT_GOOD_OCTETS"       |  Number of bytes transmitted in all the frames (Error free frames)
 "OUT_ALL_OCTETS"        |  Number of bytes transmitted in all frames (good/error frames)
 "OUT_GOOD_PKTS"         |  Number of frames transmitted without error
 "OUT_ALL_PKTS"          |  Number of frames transmitted overall (Good/Bad Frames)
 "OUT_VLAN_PKTS"         |  Number of frames transmitted that are VLAN type
 "OUT_UCAST_PKTS"        |  Number of frames transmitted with Unicast Address
 "OUT_MCAST_PKTS"        |  Number of frames transmitted with Multicast Address
 "OUT_BCAST_PKTS"        |  Number of frames transmitted with Broadcast Address
 "OUT_NON_UCAST_PKTS"    |  Number of frames transmitted with Multicast and Broadcast Address
 "OUT_ERROR_PKTS"        |  Number of frames transmitted with any error (CRC)
 "OUT_OCTETS_RATE"       |  Number of bytes transmitted in the port rate buffer
 "IN_PKTS_LT_64"         |  Number of frames received whose size is less than 64-bytes
 "IN_PKTS_EQ_64"         |  Number of frames received whose size is equal to 64-bytes
 "IN_PKTS_65_TO_127"     |  Number of frames received whose size is within the range of 65 to 127 bytes
 "IN_PKTS_128_TO_255"    |  Number of frames received whose size is within the range of 128 to 255 bytes
 "IN_PKTS_256_TO_511"    |  Number of frames received whose size is within the range of 256 to 511 bytes
 "IN_PKTS_512_TO_1023"   |  Number of frames received whose size is within the range of 512 to 1023 bytes
 "IN_PKTS_1024_TO_1518"  |  Number of frames received whose size is within the range of 1024 to 1518 bytes
 "IN_PKTS_1519_TO_2047"  |  Number of frames received whose size is within the range of 1419 to 2047 bytes
 "IN_PKTS_2048_TO_4095"  |  Number of frames received whose size is within the range of 2048 to 4095 bytes
 "IN_PKTS_4096_TO_8191"  |  Number of frames received whose size is within the range of 2048 to 4095 bytes
 "IN_PKTS_8192_TO_9215"  |  Number of frames received whose size is within the range of 4096 to 8191 bytes
 "IN_PKTS_9216"          |  Number of frames received whose size is equal to 9216.
 "IN_PKTS_RATE"          |  Number of frames received in port rate buffer
 "OUT_PKTS_LT_64"        |  Number of frames transmitted whose size is less than 64-bytes
 "OUT_PKTS_EQ_64"        |  Number of frames transmitted whose size is equal to 64-bytes
 "OUT_PKTS_65_TO_127"    |  Number of frames transmitted whose size is within the range of 65 to 127 bytes
 "OUT_PKTS_128_TO_255"   |  Number of frames transmitted whose size is within the range of 128 to 255 bytes
 "OUT_PKTS_256_TO_511"   |  Number of frames transmitted whose size is within the range of 256 to 511 bytes
 "OUT_PKTS_512_TO_1023"  |  Number of frames transmitted whose size is within the range of 512 to 1023 bytes
 "OUT_PKTS_1024_TO_1518" |  Number of frames transmitted whose size is within the range of 1024 to 1518 bytes
 "OUT_PKTS_1519_TO_2047" |  Number of frames transmitted whose size is within the range of 1419 to 2047 bytes
 "OUT_PKTS_2048_TO_4095" |  Number of frames transmitted whose size is within the range of 2048 to 4095 bytes
 "OUT_PKTS_4096_TO_8191" |  Number of frames transmitted whose size is within the range of 4096 to 8191 bytes
 "OUT_PKTS_8192_TO_9215" |  Number of frames transmitted whose size is within the range of 8912 to9215 bytes
 "OUT_PKTS_9216"         |  Number of frames transmitted whose size is equal to 9216
 "OUT_PKTS_RATE"         |  Number of frames transmitted from port rate buffer
 "IN_PAUSE_PKTS"         |  Number of frames received that are PAUSE type frames
 "OUT_PAUSE_PKTS"        |  Number of frames transmitted that are PAUSE type frames
 "IN_PFC_0_PKTS"         |  Number of frames received of Priority PAUSEtype and whose Priority#0 is set
 "IN_PFC_1_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#1 is set
 "IN_PFC_2_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#2 is set
 "IN_PFC_3_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#3 is set
 "IN_PFC_4_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#4 is set
 "IN_PFC_5_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#5 is set
 "IN_PFC_6_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#6 is set
 "IN_PFC_7_PKTS"         |  Number of frames received of Priority PAUSE type and whose Priority#7 is set
 "OUT_PFC_0_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#0 is set
 "OUT_PFC_1_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#1 is set
 "OUT_PFC_2_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#2 is set
 "OUT_PFC_3_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whosePriority#3 is set
 "OUT_PFC_4_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#4 is set
 "OUT_PFC_5_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#5 is set
 "OUT_PFC_6_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#6 is set
 "OUT_PFC_7_PKTS"        |  Number of frames transmitted of Priority PAUSE type and whose Priority#7 is set
 "IN_OVER_SIZED_PKTS"    |  Number of Frames received whose size is greater than programmed MaxFrameSize and are error free and also it counts number of frames received which exceed the programmed Jabber size and have CRC Error
 "IN_UNDER_SIZED_PKTS"   |  Number of Frames received whose size is less than MinFrameSize and are error Free
 "OUT_OVER_SIZED_PKTS"   |  Number of frames transmitted whose size is within the range of 1519 to 2047 bytes, 2048_ to 4095 bytes, 4096 to 8191 bytes, 8192 to 9215 bytes and equal to 9216 bytes.
 "IN_FRAMES_TOO_LONG"    |  Number of Frames received whose size is too long.
 "IN_PFC_0_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#0
 "IN_PFC_1_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#1
 "IN_PFC_2_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#2
 "IN_PFC_3_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#3
 "IN_PFC_4_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#4
 "IN_PFC_5_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#5
 "IN_PFC_6_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#6
 "IN_PFC_7_RX_PAUSE_DURATION" | Duration of XOFF time (in us) for Receive Priority#7
 "IN_PFC_0_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#0
 "IN_PFC_1_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#1
 "IN_PFC_2_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#2
 "IN_PFC_3_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#3
 "IN_PFC_4_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#4
 "IN_PFC_5_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#5
 "IN_PFC_6_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#6
 "IN_PFC_7_TX_PAUSE_DURATION" | Duration of XOFF time (in us) for Transmit Priority#7
 "OCTETS"                |  Number of bytes received in all the frames (Error free frames) and Number of frames transmitted overall (Good/Bad Frames)
 "PKTS"                  |  Number of frames received overall (Good/Bad Frames) and Number of frames transmitted overall (Good/Bad Frames)

## Counters related to various drop reasons

Following table describes counters dedicated to several malformed packets received or transmitted at the ports.

Counter Name	         |   Description                          |
-------------------------|----------------------------------------|
 "IF_IN_DISCARDS" | Number of frames received which are discarded for any reason.
 "IF_IN_OUTER_SAME_MAC_CHECK_DISCARDS"  |  Number of frames received which are discarded for same mac address.
 "IF_IN_OUTER_SMAC_MULTICAST_DISCARDS" |  Number of frames received which are discarded for multicast source MAC.
 "IF_IN_SMAC_MULTICAST_DISCARDS"  |  Number of frames received which are discarded for multicast source MAC.
 "IF_IN_OUTER_SMAC_ZERO_DISCARDS" |  Number of frames received which are discarded for source MAC being zero.
 "IF_IN_SMAC_ZERO_DISCARDS"  |  Number of frames received which are discarded for source MAC being zero.
 "IF_IN_OUTER_DMAC_ZERO_DISCARDS"  |  Number of frames received which are discarded for destination MAC being zero
 "IF_IN_DMAC_ZERO_DISCARDS"  |  Number of frames received which are discarded for destination MAC being zero
 "IF_IN_OUTER_IP_SRC_MULTICAST_DISCARDS" | Number of frames received which are discarded for multicast source MAC
 "IF_IN_IP_SRC_MULTICAST_DISCARDS"  |  Number of frames received which are discarded for multicast source MAC
 "IF_IN_OUTER_IP_VERSION_INVALID_DISCARDS"  |  Number of frames received which are discarded due to header checksum or bad IP version or IPv4 IHL too short
 "IF_IN_IP_VERSION_INVALID_DISCARDS" |  Number of frames received which are discarded due to header checksum or bad IP version or IPv4 IHL too short
 "IF_IN_OUTER_IP_CHECKSUM_INVALID_DISCARDS"  |   Number of frames received which are discarded due to invalid IP checksum
 "IF_IN_OUTER_IP_IHL_DISCARDS" | Number of frames received which are discarded due to invalid IPv4 IHL
 "IF_IN_IP_IHL_DISCARDS" | Number of frames received which are discarded due to invalid IPv4 IHL
 "IF_IN_OUTER_IP_TTL_ZERO_DISCARDS" | Number of frames received which are discarded due to zero IPv4 TTL
 "IF_IN_IP_TTL_ZERO_DISCARDS" | Number of frames received which are discarded due to zero IPv4 TTL
 "IF_IN_DIP_UNSPECIFIED_DISCARDS" | Number of frames received which are discarded due to unspecified destination IP
 "IF_IN_SIP_UNSPECIFIED_DISCARDS"  |  Number of frames received which are discarded due to unspecified source IP
 "IF_IN_UC_DIP_MC_DMAC_DISCARDS" | Number of frames received which are discarded because of Unicast destination IP with non unicast (multicast or broadcast) destination MAC
 "IF_IN_BC_DIP_MC_DMAC_DISCARDS" | Number of frames received which are discarded because of Broadcast destination IP with multicast destination MAC
 "IF_IN_SAME_IFINDEX_DISCARDS" | Number of frames received which are discarded because  same interface index
 "IF_IN_DIP_LOOPBACK_DISCARDS" | Number of frames received which are discarded because Destination IP is loopback address
 "IF_IN_SIP_LOOPBACK_DISCARDS"  |  Number of frames received which are discarded because source IP is loopback address
 "IF_IN_IP_CHECKSUM_INVALID_DISCARDS" | Number of packets received which are discarded due to invalid header checksum
 "IF_IN_IP_DST_LINK_LOCAL_DISCARDS" | Number of packets received which are discarded because IPv4 unicast destination IP is link local (Destination IP=169.254.0.0/16)
 "IF_IN_IP_SRC_LINK_LOCAL_DISCARDS"  | Number of packets received which are discarded because IPv4 Source IP is link local (Source IP=169.254.0.0/16)
 "IF_IN_IP_SRC_CLASS_E_DISCARDS" |  Number of packets received which are discarded because src ipv4 is class e
 "IF_IN_IP_LPM4_MISS_DISCARDS"  |  Number of packets received which are discarded due to IPv4 Routing table (LPM) unicast miss.
 "IF_IN_IP_LPM6_MISS_DISCARDS" | Number of packets received which are discarded due to IPv6 Routing table (LPM) unicast miss
 "IF_IN_IP_BLACKHOLE_ROUTE_DISCARDS" | Number of packets received which are discarded due to Black hole route (discard by route entry)
 "IF_IN_L3_PORT_RMAC_MISS_DISCARDS" | Number of packets received which are discarded due to rmac miss
 "IF_IN_L2_ANY_DISCARDS" | Number of frames received which are discarded due to any L2 pipeline drop
 "IF_IN_NEXTHOP_DISCARDS" |  Number of frames received which are discarded due to nexthop drop
 "IF_IN_MPLS_LOOKUP_MISS_DISCARD"  |   Number of frames received which are discarded due to MPLS drop
 "IF_IN_SRV6_MY_SID_DISCARDS" |  Number of packets received which are discarded due to local SID configuration or incorrect value in SRV6 packet header
 "IF_IN_NON_IP_ROUTER_MAC_DISCARDS" |  Number of packets received which are discarded due to rmac hit non ip
 "IF_IN_VLAN_DISCARDS"  |  Number of frames received which are discarded due to port vlan mapping miss
 "IF_IN_ACL_DENY_DISCARDS"  |  Number of frames received which are discarded due to ACL deny
 "IF_IN_RACL_DENY_DISCARDS"  |  Number of frames received which are discarded due to RACL deny
 "IF_IN_ACL_METER_DISCARDS" |  Number of frames received which are discarded due to ACL metering
 "IF_IN_PORT_METER_DISCARDS"| Number of frames received which are discarded due to port metering
 "IF_IN_L3_IPV4_DISABLE_DISCARDS" | Number of packets received which are discarded due to ipv4 unicast disable
 "IF_IN_L3_IPV6_DISABLE_DISCARDS" | Number of packets received which are discarded due to ipv6 unicast disable
 "IF_IN_MPLS_DISABLE_DISCARDS" | Number of frames received which are discarded due to MPLS disable
 "IF_IN_L3_LPM4_MISS_DISCARDS" | Number of packets received which are discarded due to IP LPM4 miss
 "IF_IN_L3_LPM6_MISS_DISCARDS" | Number of packets received which are discarded due to IP LPM6 miss
 "IF_IN_DMAC_RESERVED_DISCARDS"  |  Number of frames received which are discarded due to reserved DMAC
 "IF_IN_NON_ROUTABLE_DISCARDS"  |  Number of frames received which are discarded due to IGMP Non Routable
 "IF_IN_FDB_AND_BLACKHOLE_DISCARDS" | Number of frames received which are discarded due to l2 miss unicast, broadcast and multicast and blackhole route
 "IF_IN_L2_MISS_UNICAST"  | Number of frames received which are discarded due to unicast FDB table action discard
 "IF_IN_L2_MISS_MULTICAST" | Number of frames received which are discarded due to multicast FDB table empty tx list
 "IF_IN_L2_MISS_BROADCAST" |  Number of frames received which are discarded due to l2 miss broadcast
 "IF_IN_SIP_BC_DISCARDS" | Number of frames received which are discared because source IPv4 is limited broadcast
 "IF_IN_IPV6_MC_SCOPE0_DISCARD" | Number of frames received which are discared because IPv6 destination is multicast scope 0 reserved
 "IF_IN_IPV6_MC_SCOPE1_DISCARD" | Number of frames received which are discared because IPv6 destination is multicast scope 1 interface-local
 "IF_OUT_DISCARDS" |  Number of frames transmitted which are discarded for any reason
 "IBUF_DISCARDS" | Number of frames received which are discarded in Buffer Packet
 "IF_IN_STP_STATE_BLOCKING_DISCARDS" | Number of ingress frames that are discarded due to STP blocking state
 "IF_OUT_STP_STATE_BLOCKING_DISCARDS" | Number of egress frames that are discarded due to STP blocking state
 "IF_OUT_STORM_PFC_WD_DISCARDS" | Number of frames transmitted that are discarded due to PFC WD drop
 "IF_OUT_MTU_CHECK_FAIL_DISCARDS" | Number of packets transmitted that are discarded due to failure in l3 mtu check
 "IF_OUT_EGRESS_ACL_DENY_DISCARDS" | Number of packets transmitted that are discarded due to ACL deny
 "IF_OUT_EGRESS_ACL_METER_DISCARDS" | Number of packets transmitted that are discarded due to ACL meter drop
 "IF_OUT_EGRESS_PORT_METER_DISCARDS" | Number of packets transmitted that are discarded due to port meter drop
 "INGRESS_TM_DISCARDS" | Number of packets received that are dropped due to buffers
 "EGRESS_TM_DISCARDS" | Number of packets transmitted that are discarded due to buffers
 "IN_CURR_OCCUPANCY_BYTES" |  Get in port current occupancy in bytes [uint64_t]
 "OUT_CURR_OCCUPANCY_BYTES" |  Get out port current occupancy in bytes [uint64_t]
 "WRED_GREEN_DROPPED_PACKETS" | Get/set WRED green packet count [uint64_t]
 "WRED_YELLOW_DROPPED_PACKETS" | Get/set WRED yellow packet count [uint64_t]
 "WRED_RED_DROPPED_PACKETS" |  Get/set WRED red packet count [uint64_t]
 "WRED_GREEN_DROPPED_BYTES" | Get/set WRED green byte count [uint64_t]
 "WRED_YELLOW_DROPPED_BYTES" | Get/set WRED yellow byte count [uint64_t]
 "WRED_RED_DROPPED_BYTES" | Get/set WRED red byte count [uint64_t]
 "WRED_GREEN_ECN_MARKED_PACKETS" | Get/set WRED green packets marked by ECN count [uint64_t]
 "WRED_YELLOW_ECN_MARKED_PACKETS"| Get/set WRED yellow packets marked by ECN count [uint64_t]
 "WRED_RED_ECN_MARKED_PACKETS" | Get/set WRED red packets marked by ECN count [uint64_t]
 "SC_UCAST_RED_PACKETS" | Get/set unicast RED packet count
 "SC_UCAST_GREEN_PACKETS" | Get/set unicast green packet count
 "SC_MCAST_RED_PACKETS" | Get/set multicast red packet count
 "SC_MCAST_GREEN_PACKETS" | Get/set multicast green packet count
 "SC_BCAST_RED_PACKETS" | Get/set broadcast red packet count
 "SC_BCAST_GREEN_PACKETS" | Get/set broadcast green packet count


## IP Port Counters

IP Port Counters are enabled in Y4 profile for Tofino 2 Switch hardware. It displays statistics of Ipv4, Ipv6 counters information for each port. Based on the priorities shown below, p4 programs one set of entries for every local port in the pipe.

Priority |  	Rule
---------|---------------------
0	     |   Ipv4 discards
1	     |   Ipv4 non unicast
2	     |   Ipv4 unicast
3	     |   Ipv6 discards
4	     |   Ipv6 broadcast
5	     |   Ipv6 multicast
6	     |   Ipv6 unicast

Following entries are programmed into the table:

`port#	is_ipv4	 is_ipv6  drop  copy_to_cpu  eth.dst_addr  priority`

Counters descriptions are shown in below table.

Counter Name	         |   Description  |
-------------------------|---------------------------------------------------------------------|
"IP_IN_RECEIVES" |	It computes the sum of Ipv4 discard packets, ipv4 non unicast packets and ipv4 unicast packets
"IP_IN_OCTETS"	| ingress octets received at the port. Sum of octets of ipv4 discards, ipv4 non unicast, ipv4 unicast.
"IP_IN_UCAST_PKTS"|	Ingress ipv4 unicast packets
"IP_IN_NON_UCAST_PKTS" |	Sum of Ingress ipv4 multicast and broadcast packets
"IP_IN_DISCARDS" |	Ingress ipv4 packets dropped at the port
"IP_OUT_OCTETS"	| egress octets sent at the port. Sum of octets of ipv4 discards, ipv4 non unicast, ipv4 unicast
"IP_OUT_UCAST_PKTS" |	Egress ipv4 unicast packets
"IP_OUT_NON_UCAST_PKTS"|	Sum of Egress ipv4 multicast and broadcast packets
"IP_OUT_DISCARDS" |	Egress ipv4 packets dropped at the port
"IPV6_IN_RECEIVES" |	It computes the sum of Ipv6 discard packets, ipv6 multicast, ipv6 broadcast packets and ipv6 unicast packets
"IPV6_IN_OCTETS" |	ingress octets received at the port. Sum of octets of ipv6 discards, ipv6 broadcast, ipv6 multicast and ipv6 unicast.
"IPV6_IN_UCAST_PKTS" |	Ingress ipv6 unicast packets
"IPV6_IN_NON_UCAST_PKTS" |	Sum of Ingress ipv6 multicast and broadcast packets
"IPV6_IN_MCAST_PKTS" |	Ingress ipv6 multicast packets.
"IPV6_IN_DISCARDS" |	Ingress ipv6 packets dropped at the port
"IPV6_OUT_OCTETS" |	egress octets received at the port. Sum of octets of ipv6 discards, ipv6 broadcast, ipv6 multicast and ipv6 unicast.
"IPV6_OUT_UCAST_PKTS" |	Egress ipv6 unicast packets
"IPV6_OUT_NON_UCAST_PKTS" |	Sum of Egress ipv6 multicast and broadcast packets
"IPV6_OUT_MCAST_PKTS" |	Egress ipv6 multicast packets
"IPV6_OUT_DISCARDS"	 | Egress ipv6 packets dropped at the port

## Code

BMAI-SAI implementation for port counters are in the following files.
```
api/switch_tna/port.cpp
schema/appObj/port.json
schema/asicObj/port.json
sai/saiport.cpp
```
