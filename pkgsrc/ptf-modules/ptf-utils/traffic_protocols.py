################################################################################
 #  Copyright (C) 2024 Intel Corporation
 #
 #  Licensed under the Apache License, Version 2.0 (the "License");
 #  you may not use this file except in compliance with the License.
 #  You may obtain a copy of the License at
 #
 #  http://www.apache.org/licenses/LICENSE-2.0
 #
 #  Unless required by applicable law or agreed to in writing,
 #  software distributed under the License is distributed on an "AS IS" BASIS,
 #  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 #  See the License for the specific language governing permissions
 #  and limitations under the License.
 #
 #
 #  SPDX-License-Identifier: Apache-2.0
################################################################################


from ostinato.core import ost_pb

import ostinato.protocols.arp_pb2 as arp_pb2
import ostinato.protocols.dot2llc_pb2 as dot2llc_pb2
import ostinato.protocols.dot2snap_pb2 as dot2snap_pb2
import ostinato.protocols.dot3_pb2 as dot3_pb2
import ostinato.protocols.eth2_pb2 as eth2_pb2
import ostinato.protocols.gmp_pb2 as gmp_pb2
import ostinato.protocols.icmp_pb2 as icmp_pb2
import ostinato.protocols.igmp_pb2 as igmp_pb2
import ostinato.protocols.ip4over4_pb2 as ip4over4_pb2
import ostinato.protocols.ip4over6_pb2 as ip4over6_pb2
import ostinato.protocols.ip4_pb2 as ip4_pb2
import ostinato.protocols.ip6over4_pb2 as ip6over4_pb2
import ostinato.protocols.ip6over6_pb2 as ip6over6_pb2
import ostinato.protocols.ip6_pb2 as ip6_pb2
import ostinato.protocols.llc_pb2 as llc_pb2
import ostinato.protocols.mac_pb2 as mac_pb2
import ostinato.protocols.mld_pb2 as mld_pb2
import ostinato.protocols.payload_pb2 as payload_pb2
import ostinato.protocols.protocol_pb2 as protocol_pb2
import ostinato.protocols.snap_pb2 as snap_pb2
import ostinato.protocols.svlan_pb2 as svlan_pb2
import ostinato.protocols.tcp_pb2 as tcp_pb2
import ostinato.protocols.udp_pb2 as udp_pb2
import ostinato.protocols.userscript_pb2 as userscript_pb2
import ostinato.protocols.vlan_pb2 as vlan_pb2
import ostinato.protocols.vlanstack_pb2 as vlanstack_pb2

import socket
import struct
from binascii import hexlify

def convert_ipv4_string_to_int(ip_addr):
    """Convert IPv4 address to integer used as input in Ostinato.
    socket module will do the ipv4 format check and raise an error

    Example:
        '1.1.1.1' -> 0x01010101, or 16843009 (decimal)

    Args:
        ip_addr (string): IPv4 address

    Returns:
        integer: IPv4 address in integer format
    """

    return struct.unpack("!I", socket.inet_aton(ip_addr))[0]

def convert_ipv6_string_to_int(ip_addr):
    """Convert IPv6 address to dict of lo and hi integer used as input in Ostinato.
    socket module will do the ipv6 format check and raise an error

    Example:
        2001::1 -> ipv6['hi'] = 0x2001000000000000, or 2306124484190404608 (decimal)
                   ipv6['lo'] = 0x0000000000000001, or 1 (decimal)
    Args:
        ip_addr (string): IPv6 address string (short form is also accepted)

    Returns:
        dictionary: ipv6['hi'] IPv6 high bytes in integer
                    ipv6['lo'] IPv6 low bytes in integer
    """

    ipv6 = dict()

    ipv6_int = int(hexlify(socket.inet_pton(socket.AF_INET6, ip_addr)), 16)
    ipv6_hex_string = hex(ipv6_int).lstrip('0x').rstrip('L')

    ipv6['hi'] = int(ipv6_hex_string[0:16], 16)
    ipv6['lo'] = int(ipv6_hex_string[16:32], 16)
    return ipv6

def convert_mac_string_to_int(mac):
    """Convert MAC address to integer used as input in Ostinato.
    Does not check for valid mac address, but it takes : . - as mac delimiter
    i.e. xxxx.xxxx.xxxx or xx-xx-xx-xx-xx-xx or xx:xx:xx:xx:xx:xx

    Example:
        00:11:22:33:44:55 -> 0x001122334455, or 73588229205 (decimal)

    Args:
        mac (string): MAC address string

    Returns:
        integer: Mac address in integer format
    """

    mac = mac.replace(':', '')
    mac = mac.replace('.', '')
    mac = mac.replace('-', '')
    return int(mac, 16)

def convert_ipv4_int_to_string(ip_addr):
    """Convert an IPv4 integer to valid IPv4 string used in Wireshark filter.
    socket module will do the some format check and raise an error

    Example:
        0x01010101 or 16843009 (decimal) -> '1.1.1.1'

    Args:
        ip_addr (integer): IPv4 address integer

    Returns:
        string: IPv4 address
    """

    return socket.inet_ntoa(struct.pack("!I", ip_addr))

def convert_ipv6_int_to_string(ipv6_hi, ipv6_lo):
    """Convert IPv6 hi and lo integer to IPv6 address string used in Wireshark filter.
    socket module will some ipv6 format check and raise an error

    Example:
         ipv6_hi = 0x2001000000000000 or 2306124484190404608 (decimal)
         ipv6_lo = 0x0000000000000001 or 1 (decimal)
         string returned = '2001::1'

    Args:
        ipv6_hi (int): IPv6 address integer for the hi 8 bytes
        ipv6_lo (int): IPv6 address integer for the lo 8 bytes

    Returns:
        string: IPv6 address
    """

    ipv6_hex_hi = struct.pack("!Q", ipv6_hi)
    ipv6_hex_lo = struct.pack("!Q", ipv6_lo)
    return socket.inet_ntop(socket.AF_INET6, ipv6_hex_hi + ipv6_hex_lo)

def convert_mac_int_to_string(mac):
    """Convert a MAC integer to MAC address string used in Wireshark filter
    Example:
        0x001122334455, or 73588229205 (decimal) -> 00:11:22:33:44:55

    Args:
        mac (int): MAC address integer

    Returns:
        string: MAC address string in xx:xx:xx:xx:xx:xx format
    """

    mac_hex = hex(mac).lstrip('0x').rstrip('L').zfill(12)
    return mac_hex[0:2] + ":" + mac_hex[2:4] + ":" + mac_hex[4:6] + ":" + \
           mac_hex[6:8] + ":"+ mac_hex[8:10] + ":" + mac_hex[10:12]

class TrafficProtocolMac:
    """Class that holds the Mac Protocol attributes

    Attributes:
        dst_mac_mode(str):   "fixed"(default), "increment", "decrement"
        dst_mac(int):        destinaton mac (default=0x0012345678)
        dst_mac_count(int):  number of destination mac when mode is not fixed (default=16)
        dst_mac_step(int):   destination mac step when mode is not fixed (default=1)
        src_mac_mode(str):   "fixed"(default), "increment", "decrement"
        src_mac(int):        destinaton mac (default=0x0012345678)
        src_mac_count(int):  number of source mac when mode is not fixed (default=16)
        src_mac_step(int):   source mac step when mode is not fixed (default=1)
    """
    def __init__(self):
        ## Can be fixed, increment, or decrement
        ## Ostinato variables: p.Extensions[mac_pb2.mac]. 'e_mm_dec', 'e_mm_fixed', 'e_mm_inc'
        self.dst_mac_mode = "fixed"
        self.dst_mac = 0x0012345678
        self.dst_mac_count = 16
        self.dst_mac_step = 1
        ## Can be fixed, increment, or decrement
        ## Ostinato variables: p.Extensions[mac_pb2.mac]. 'e_mm_dec', 'e_mm_fixed', 'e_mm_inc'
        self.src_mac_mode = "fixed"
        self.src_mac = 0x0034567890
        self.src_mac_count = 16
        self.src_mac_step = 1

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kMacFieldNumber

        ost_mac = ost_p.Extensions[mac_pb2.mac]

        if self.src_mac_mode == "increment":
            ost_mac.src_mac_mode = ost_mac.e_mm_inc
        elif self.src_mac_mode == "decrement":
            ost_mac.src_mac_mode = ost_mac.e_mm_dec
        else:
            ost_mac.src_mac_mode = ost_mac.e_mm_fixed
        ost_mac.src_mac          = self.src_mac
        ost_mac.src_mac_count    = self.src_mac_count
        ost_mac.src_mac_step     = self.src_mac_step

        if self.dst_mac_mode == "increment":
            ost_mac.dst_mac_mode = ost_mac.e_mm_inc
        elif self.dst_mac_mode == "decrement":
            ost_mac.dst_mac_mode = ost_mac.e_mm_dec
        else:
            ost_mac.dst_mac_mode = ost_mac.e_mm_fixed
        ost_mac.dst_mac          = self.dst_mac
        ost_mac.dst_mac_count    = self.dst_mac_count
        ost_mac.dst_mac_step     = self.dst_mac_step

    def set_src_mac(self, mac, mac_mode="fixed", mac_step=1, mac_count=1):
        """
        Method to set the source mac.
        It also does a type check and automatically calls
        convert_mac_string_to_int if the input is string type
        """

        if isinstance(mac, basestring):
            self.src_mac = convert_mac_string_to_int(mac)
        else:
            self.src_mac = mac
        self.src_mac_mode = mac_mode
        self.src_mac_step = mac_step
        self.src_mac_count = mac_count

    def set_dst_mac(self, mac, mac_mode="fixed", mac_step=1, mac_count=1):
        """
        Method to set the destination mac.
        It also does a type check and automatically calls
        convert_mac_string_to_int if the input is string type
        """

        if isinstance(mac, basestring):
            self.dst_mac = convert_mac_string_to_int(mac)
        else:
            self.dst_mac = mac
        self.dst_mac_mode = mac_mode
        self.dst_mac_step = mac_step
        self.dst_mac_count = mac_count

    def increment_src_mac(self, mac, mac_step=1, mac_count=1):
        if isinstance(mac, basestring):
            self.src_mac = convert_mac_string_to_int(mac)
        else:
            self.src_mac = mac
        self.src_mac_mode = "increment"
        self.src_mac_step = mac_step
        self.src_mac_count = mac_count

    def increment_dst_mac(self, mac, mac_step=1, mac_count=1):
        if isinstance(mac, basestring):
            self.dst_mac = convert_mac_string_to_int(mac)
        else:
            self.dst_mac = mac
        self.dst_mac_mode = mac_mode
        self.dst_mac_step = mac_step
        self.dst_mac_count = mac_count

class TrafficProtocolEth2:
    """Class that holds the Ethernet2 Protocol attributes

    Attributes:
        is_override_type(bool): True to override the eth2 type, False (default) to let Ostinato decide appropriately
        type(int):              eth2 type (default 0x800, only applicable if override is True)
    """

    def __init__(self):
        self.is_override_type = False
        self.type = 0x800

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kEth2FieldNumber

        ost_eth2 = ost_p.Extensions[eth2_pb2.eth2]
        ost_eth2.is_override_type = self.is_override_type
        ost_eth2.type             = self.type


class TrafficProtocolVlan:
    """Class that holds the Vlan / Dot1Q Protocol attributes

    Attributes:
        is_override_tpid(bool): True to override tpid, False (default) to use default 0x8100
        tpid(int):              TPID (default=0x8100, only takes effect if override = True)
        vlan_id(int):           Vlan tag id

    Args:
        vlan_id(int):           Vlan tag id
    """

    def __init__(self, vlan_id):
        self.is_override_tpid = False
        self.tpid = 0x8100
        self.vlan_id = vlan_id

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kVlanFieldNumber

        ost_vlan = ost_p.Extensions[vlan_pb2.vlan]
        ost_vlan.vlan_tag         = self.vlan_id
        ost_vlan.is_override_tpid = self.is_override_tpid
        ost_vlan.tpid             = self.tpid

class TrafficProtocolVxlan:
    """Class that holds the Vxlan Protocol attributes

    Attributes:
        vni(int):           VNI tag
        flags(int):         Flags
    Args:
        vni(int):           VNI tag
    """

    def __init__(self, vni):
        self.vni = vni           ;# set VNI tag
        self.flags = 0x08        ;# set I bit

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kUserScriptFieldNumber

        ost_p.Extensions[userscript_pb2.userScript].program = 'protocol.name = "VxLAN"\x0a\x0aprotocol.protocolFrameSize = function() {\x0a\x0a    return 8;\x0a\x0a}\x0a\x0aprotocol.protocolFrameValue = function(index) {\x0a\x0a    var flags  = 0x08;\x0a    var vni  = ' + '{0}'.format(self.vni) + ';\x0a\x0a    var pfv = new Array(8);\x0a\x0a    pfv[0] = flags;\x0a    pfv[1] = 0;\x0a    pfv[2] = 0;\x0a    pfv[3] = 0;\x0a\x0a    pfv[4] = ( vni >> 16 ) & 0xFF;\x0a    pfv[5] = ( vni >> 8 ) & 0xFF;\x0a    pfv[6] = vni & 0xFF;\x0a    pfv[7] = 0;\x0a\x0a    return pfv;\x0a}\x0a\x0aprotocol.protocolId = function() {\x0a\x0areturn 0x4789;\x0a\x0a}'


class TrafficProtocolStackedVlan:
    """Class that holds the Stacked Vlan Protocol attributes

    Attributes:
        outer_is_override_tpid(bool): True to override tpid, False (default) to use default 0x88a8
        outer_tpid(int):              Outer TPID (default=0x8100, only takes effect if override = True)
        outer_vlan_id(int):           Outer Vlan tag id
        inner_is_override_tpid(bool): True to override tpid, False (default) to use default 0x88a8
        inner_tpid(int):              Inner TPID (default=0x8100, only takes effect if override = True)
        inner_vlan_id(int):           Inner Vlan tag id

    Args:
        outer_vlan_id(int):           Outer Vlan tag id
        inner_vlan_id(int):           Inner Vlan tag id
    """

    def __init__(self, outer_vlan_id, inner_vlan_id):
        self.outer_is_overrid_tpid = True
        self.outer_tpid = 0x88a8
        self.outer_vlan_id = outer_vlan_id
        self.inner_is_override_tpid = False
        self.inner_tpid = 0x8100
        self.inner_vlan_id = inner_vlan_id

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kVlanStackFieldNumber

        ost_vlanstack = ost_p.Extensions[svlan]
        ost_vlanstack.is_override_type = self.outer_is_override_type
        ost_vlanstack.tpid             = self.outer_tpid
        ost_vlanstack.vlan_tag         = self.outer_vlan_id

        ost_vlan = ost_p.Extensions[vlan]
        ost_vlan.is_override_type = self.inner_is_override_type
        ost_vlan.tpid             = self.inner_tpid
        ost_vlan.vlan_tag         = self.inner_vlan_id


class TrafficProtocolIpv4:
    """Class that holds the IPv4 Protocol attributes

    """

    def __init__(self):
        self.is_override_ver = False
        self.is_override_hdrlen = False
        self.ver_hdrlen = 0
        self.is_override_totlen = False
        self.totlen = 0
        self.is_override_proto = False
        self.proto = 0
        self.is_override_cksum = False
        self.cksum = 0
        self.tos = 0
        self.ttl = 128

        ## flags value: DF bit = 0x10, MF bit = 0x01, DF + MF bit = 0x11
        self.flags = 0
        self.frag_ofs = 0

        ## fixed, increment, decrement, random
        ## 'e_im_dec_host', 'e_im_fixed', 'e_im_inc_host', 'e_im_random_host'
        self.src_ip_mode = "fixed"
        self.src_ip = 0
        self.src_ip_count = 16
        self.src_ip_mask = 0xffffff00

        ## fixed, increment, decrement, random
        ## 'e_im_dec_host', 'e_im_fixed', 'e_im_inc_host', 'e_im_random_host'
        self.dst_ip_mode = "fixed"
        self.dst_ip = 0
        self.dst_ip_count = 16
        self.dst_ip_mask = 0xffffff00

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kIp4FieldNumber

        ost_ipv4 = ost_p.Extensions[ip4_pb2.ip4]
        ost_ipv4.is_override_ver     = self.is_override_ver
        ost_ipv4.is_override_hdrlen  = self.is_override_hdrlen
        ost_ipv4.ver_hdrlen          = self.ver_hdrlen
        ost_ipv4.is_override_totlen  = self.is_override_totlen
        ost_ipv4.totlen              = self.totlen
        ost_ipv4.is_override_proto   = self.is_override_proto
        ost_ipv4.proto               = self.proto
        ost_ipv4.is_override_cksum   = self.is_override_cksum
        ost_ipv4.cksum               = self.cksum
        ost_ipv4.tos                 = self.tos
        ost_ipv4.ttl                 = self.ttl

        ost_ipv4.flags               = self.flags
        ost_ipv4.frag_ofs            = self.frag_ofs

        if self.src_ip_mode == "increment":
            ost_ipv4.src_ip_mode     = ost_ipv4.e_im_inc_host
        elif self.src_ip_mode == "decrement":
            ost_ipv4.src_ip_mode     = ost_ipv4.e_im_dec_host
        elif self.src_ip_mode == "random":
            ost_ipv4.src_ip_mode     = ost_ipv4.e_im_random_host
        else:
            ost_ipv4.src_ip_mode     = ost_ipv4.e_im_fixed
        ost_ipv4.src_ip              = self.src_ip
        ost_ipv4.src_ip_count        = self.src_ip_count
        ost_ipv4.src_ip_mask         = self.src_ip_mask

        if self.dst_ip_mode == "increment":
            ost_ipv4.dst_ip_mode     = ost_ipv4.e_im_inc_host
        elif self.dst_ip_mode == "decrement":
            ost_ipv4.dst_ip_mode     = ost_ipv4.e_im_dec_host
        elif self.dst_ip_mode == "random":
            ost_ipv4.dst_ip_mode     = ost_ipv4.e_im_random_host
        else:
            ost_ipv4.dst_ip_mode     = ost_ipv4.e_im_fixed
        ost_ipv4.dst_ip              = self.dst_ip
        ost_ipv4.dst_ip_count        = self.dst_ip_count
        ost_ipv4.dst_ip_mask         = self.dst_ip_mask


    def set_src_ip(self, ip, ip_mode="fixed", ip_step=1, ip_count=1):
        """
        Method to set the source ip
        It also does a type check and automatically calls
        convert_ipv4_string_to_int if the input is string type
        """

        if isinstance(ip, basestring):
            self.src_ip = convert_ipv4_string_to_int(ip)
        else:
            self.src_ip = ip
        self.src_ip_mode = ip_mode
        self.src_ip_step = ip_step
        self.src_ip_count = ip_count

    def set_dst_ip(self, ip, ip_mode="fixed", ip_step=1, ip_count=1):
        """
        Method to set the destination ip
        It also does a type check and automatically calls
        convert_ipv4_string_to_int if the input is string type
        """

        if isinstance(ip, basestring):
            self.dst_ip = convert_ipv4_string_to_int(ip)
        else:
            self.dst_ip = ip
        self.dst_ip_mode = ip_mode
        self.dst_ip_step = ip_step
        self.dst_ip_count = ip_count

    def set_ttl(self, ttl):
        """
        Method to set the ttl
        """
        self.ttl = ttl

    def increment_src_ip(self, ip, ip_step=1, ip_count=1):
        if isinstance(ip, basestring):
            self.src_ip = convert_ip_string_to_int(ip)
        else:
            self.src_ip = ip
        self.src_ip_mode = "increment"
        self.src_ip_step = ip_step
        self.src_ip_count = ip_count

    def increment_dst_ip(self, ip, ip_step=1, ip_count=1):
        if isinstance(ip, basestring):
            self.dst_ip = convert_ip_string_to_int(ip)
        else:
            self.dst_ip = ip
        self.dst_ip_mode = "increment"
        self.dst_ip_step = ip_step
        self.dst_ip_count = ip_count

class TrafficProtocolIpv6:
    """Class that holds the IPv6 Protocol attributes

    """

    def __init__(self):
        self.is_override_version = False
        self.is_override_payload_length = False
        self.is_override_next_header = False
        self.version = 6
        self.traffic_class = 0
        self.flow_label = 0
        self.payload_length = 6
        self.next_header = 59
        self.hop_limit = 127
        self.src_addr_hi = 0x0
        self.src_addr_lo = 0x0
        self.src_addr_mode = "fixed"
        self.src_addr_count = 16
        self.dst_addr_hi = 0x0
        self.dst_addr_lo = 0x0
        self.dst_addr_mode = "fixed"
        self.dst_addr_count = 16

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kIp6FieldNumber

        ost_ipv6 = ost_p.Extensions[ip6_pb2.ip6]
        ost_ipv6.is_override_version        = self.is_override_version
        ost_ipv6.is_override_payload_length = self.is_override_payload_length
        ost_ipv6.is_override_next_header    = self.is_override_next_header
        ost_ipv6.version                    = self.version
        ost_ipv6.traffic_class              = self.traffic_class
        ost_ipv6.flow_label                 = self.flow_label
        ost_ipv6.payload_length             = self.payload_length
        ost_ipv6.next_header                = self.next_header
        ost_ipv6.hop_limit                  = self.hop_limit

        if self.src_addr_mode == "increment":
            ost_ipv6.src_addr_mode          = ost_ipv6.kIncHost
        elif self.src_addr_mode == "decrement":
            ost_ipv6.src_addr_mode          = ost_ipv6.kDecHost
        elif self.src_addr_mode == "random":
            ost_ipv6.src_addr_mode          = ost_ipv6.kRandomHost
        else:
            ost_ipv6.src_addr_mode          = ost_ipv6.kFixed
        ost_ipv6.src_addr_hi                = self.src_addr_hi
        ost_ipv6.src_addr_lo                = self.src_addr_lo
        ost_ipv6.src_addr_count             = self.src_addr_count

        if self.dst_addr_mode == "increment":
            ost_ipv6.dst_addr_mode          = ost_ipv6.kIncHost
        elif self.dst_addr_mode == "decrement":
            ost_ipv6.dst_addr_mode          = ost_ipv6.kDecHost
        elif self.dst_addr_mode == "random":
            ost_ipv6.dst_addr_mode          = ost_ipv6.kRandomHost
        else:
            ost_ipv6.dst_addr_mode          = ost_ipv6.kFixed
        ost_ipv6.dst_addr_hi                = self.dst_addr_hi
        ost_ipv6.dst_addr_lo                = self.dst_addr_lo
        ost_ipv6.dst_addr_count             = self.dst_addr_count

    def set_src_ip(self, ip, ip_mode="fixed", ip_step=1, ip_count=1):
        """
        Method to set the source ip.
        It also does a type check and automatically calls
        convert_ipv6_string_to_int if the input is string type
        """

        if isinstance(ip, basestring):
            ipv6 = convert_ipv6_string_to_int(ip)
            self.src_addr_hi = ipv6['hi']
            self.src_addr_lo = ipv6['lo']
        else:
            self.src_addr_hi = int(hex(ip)[2:18], 16)
            self.src_addr_lo = int(hex(ip)[18:34], 16)
            self.src_ip = ip

        self.src_ip_mode = ip_mode
        self.src_ip_step = ip_step
        self.src_ip_count = ip_count


    def set_dst_ip(self, ip, ip_mode="fixed", ip_step=1, ip_count=1):
        """
        Method to set the destination ip.
        It also does a type check and automatically calls
        convert_ipv4_string_to_int if the input is string type
        """

        if isinstance(ip, basestring):
            ipv6 = convert_ipv6_string_to_int(ip)
            self.dst_addr_hi = ipv6['hi']
            self.dst_addr_lo = ipv6['lo']
        else:
            self.dst_addr_hi = int(hex(ip)[2:18], 16)
            self.dst_addr_lo = int(hex(ip)[18:34], 16)
            self.dst_ip = ip

        self.dst_ip_mode = ip_mode
        self.dst_ip_step = ip_step
        self.dst_ip_count = ip_count

    def set_hlim(self, hlim):
        """
        Method to set the ipv6 hop limit.
        """
        self.hop_limit = hlim


class TrafficProtocolTcp:
    """Class that holds the TCP Protocol attributes

    """

    def __init__(self):
        self.is_override_src_port = False
        self.src_port = 0
        self.is_override_dst_port = False
        self.dst_port = 0
        self.is_override_hdrlen = False
        self.hdrlen_rsvd = 80
        self.is_override_cksum = False
        self.cksum = 0
        self.seq_num = 129018
        self.ack_num = 0
        self.window = 1024
        self.urg_ptr = 0
        # TCP flags:
        # 0x000001 FIN
        # 0x000010 SYN
        # 0x000100 RST
        # 0x001000 PSH
        # 0x010000 ACK
        # 0x100000 URG
        self.flags = 0x000010

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kTcpFieldNumber

        ost_tcp = ost_p.Extensions[tcp_pb2.tcp]
        ost_tcp.is_override_src_port    = self.is_override_src_port
        ost_tcp.src_port                = self.src_port
        ost_tcp.is_override_dst_port    = self.is_override_dst_port
        ost_tcp.dst_port                = self.dst_port
        ost_tcp.is_override_hdrlen      = self.is_override_hdrlen
        ost_tcp.hdrlen_rsvd             = self.hdrlen_rsvd
        ost_tcp.is_override_cksum       = self.is_override_cksum
        ost_tcp.cksum                   = self.cksum
        ost_tcp.seq_num                 = self.seq_num
        ost_tcp.ack_num                 = self.ack_num
        ost_tcp.window                  = self.window
        ost_tcp.urg_ptr                 = self.urg_ptr
        ost_tcp.flags                   = self.flags

class TrafficProtocolUdp:
    """Class that holds the UDP Protocol attributes

    """

    def __init__(self):
        self.is_override_src_port = False
        self.src_port = 0
        self.is_override_dst_port = False
        self.dst_port = 0
        self.is_override_totlen = False
        self.totlen = 26
        self.is_override_cksum = False
        self.cksum = 0

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kUdpFieldNumber

        ost_udp = ost_p.Extensions[udp_pb2.udp]
        ost_udp.is_override_src_port    = self.is_override_src_port
        ost_udp.src_port                = self.src_port
        ost_udp.is_override_dst_port    = self.is_override_dst_port
        ost_udp.dst_port                = self.dst_port
        ost_udp.is_override_totlen      = self.is_override_totlen
        ost_udp.totlen                  = self.totlen
        ost_udp.is_override_cksum       = self.is_override_cksum
        ost_udp.cksum                   = self.cksum

class TrafficProtocolIcmp:
    """Class that holds the ICMP Protocol attributes

    """

    def __init__(self):
        ## 4 for icmpv4, 6 for icmpv6
        self.icmp_version = 4

        # icmp type 0 - echo reply
        # icmp type 3 - destination unreachable
        # icmp type 4 - source quench
        # icmp type 5 - redirect
        # icmp type 8 - echo request
        # icmp type 11 - time exceeded
        # icmp type 12 - parameter problem
        # icmp type 13 - timestamp request
        # icmp type 14 - timestamp reply
        # icmp type 15 - information request
        # icmp type 16 - information reply
        # icmp type 17 - address mask request
        # icmp type 18 - address mask reply
        self.type = 8

        self.code = 0
        self.is_override_checksum = False
        self.checksum = 0
        self.identifier = 1234
        self.sequence = 0

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kIcmpFieldNumber

        ost_icmp = ost_p.Extensions[icmp_pb2.icmp]
        ost_icmp.icmp_version           = self.icmp_version
        ost_icmp.type                   = self.type
        ost_icmp.code                   = self.code
        ost_icmp.is_override_checksum   = self.is_override_checksum
        ost_icmp.checksum               = self.checksum
        ost_icmp.identifier             = self.identifier
        ost_icmp.sequence               = self.sequence

class TrafficProtocolIgmp:
    """Class that holds the IGMP Protocol attributes

    """

    def __init__(self):
        self.is_override_checksum = False
        self.checksum = 0
        ## unit = 1/10s ##
        self.max_response_time = 100

        # IGMPv1 query type = 0x11 (17)
        # IGMPv1 report type = 0x12 (18)
        # IGMPv2 query type = 0xff11
        # IGMPv2 report type = 0x16 (22)
        # IGMPv2 leave type = 0x17 (23)
        # IGMPv3 query type = 0xfe11
        # IGMPv3 report type = 0x22 (34)
        self.type = 0xff11

        # "fixed", "increment", "decrement", "random"
        self.group_mode = "fixed"
        self.group_address = 0xef010101
        self.group_count = 16
        self.group_prefix = 24
        # s_flag -> suppress router processing
        self.s_flag = False
        self.qrv = 2
        self.qqi = 125
        self.is_override_source_count = False
        self.source_count = 0
        self.source_list = []
        self.is_override_group_record_count = False
        self.group_record_count = 0
        # "reserved", "is_include", "is_exclude", "to_include", "to_exclude", "allow_new", "block_old"
        self.group_records_list[0].type = "is_include"
        self.group_records_list[0].group_address = 0xef010101
        self.group_records_list[0].source_list = []
        self.group_records_list[0].is_override_source_count = False
        self.group_records_list[0].source_count = 0
        self.group_records_list[0].aux_data = ""
        self.group_records_list[0].is_override_aux_data_length = False
        self.group_records_list[0].aux_data_length = 0

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kIgmpFieldNumber

        ost_igmp = ost_p.Extensions[igmp_pb2.igmp]
        ost_igmp.is_override_checksum = self.is_override_checksum
        ost_igmp.checksum = self.checksum
        ost_igmp.max_response_time = self.max_response_time
        ost_igmp.type = self.type
        if self.group_mode == "increment":
            ost_igmp.group_mode = ost_igmp.kIncrementGroup
        elif self.group_mode == "decrement":
            ost_igmp.group_mode = ost_igmp.kDecrementGroup
        elif self.group_mode == "random":
            ost_igmp.group_mode = ost_igmp.kRandomGroup
        else:
            ost_igmp.group_mode = ost_igmp.kFixed

        ost_igmp.group_address = self.group_address
        ost_igmp.group_count = self.group_count
        ost_igmp.group_prefix = self.group_prefix
        ost_igmp.s_flag = self.s_flag
        ost_igmp.qrv = self.qrv
        ost_igmp.qqi = self.qqi
        ost_igmp.is_override_source_count = self.is_override_source_count
        ost_igmp.source_count = self.source_count
        ost_igmp.is_override_group_record_count = self.is_override_group_record_count
        ost_igmp.group_record_count = self.group_record_count

        for group_record in self.group_records_list:
            ost_group_record = ost_p.Extensions[igmp].group_records.add()

            if group_record.type == "reserved":
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kReserved
            elif group_record.type == "is_exclude":
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kIsExclude
            elif group_record.type == "to_include":
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kToInclude
            elif group_record.type == "to_exclude":
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kToExclude
            elif group_record.type == "allow_new":
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kAllowNew
            elif group_record.type == "block_old":
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kIsInclude
            else:
                ost_group_record.tpye = gmp_pb2.Gmp.GroupRecord.kReserved
            ost_group_record.group_address.v4 = group_record.group_address

            for source_ip in group_record.source_list:
                ost_group_source = ost_group_record.sources.add()
                ost_group_source.v4 = source_ip

            ost_group_record.is_override_source_count = group_record.is_override_source_count
            ost_group_record.source_count = group_record.source_count
            ost_group_record.aux_data = group_record.aux_data
            ost_group_record.is_override_aux_data_length = group_record.is_ovverid_aux_data_length
            ost_group_record.aux_data_length = group_record.aux_data_length

class TrafficProtocolArp:
    """Class that holds the ARP Protocol attributes

    """

    def __init__(self):
        # op_code 1 = ARP Request
        # op_code 2 = ARP Reply
        self.op_code = 1

        ## fixed, increment, decrement
        ## 'kFixed', 'kIncrement', 'kDecrement'
        self.sender_hw_addr_mode = "fixed"
        self.sender_hw_addr = 0x0
        self.sender_hw_addr_count = 16

        ## fixed, increment, decrement, random
        ## 'kFixedHost', 'kIncrementHost', 'kDecrementHost', 'kRandomHost'
        self.sender_proto_addr_mode = "fixed"
        self.sender_proto_addr = 0x0
        self.sender_proto_addr_count = 16
        self.sender_proto_addr_mask = 0xffffff00

        ## fixed, increment, decrement
        ## 'kFixed', 'kIncrement', 'kDecrement'
        self.target_hw_addr_mode = "fixed"
        self.target_hw_addr = 0x0
        self.target_hw_addr_count = 16

        ## fixed, increment, decrement, random
        ## 'kFixedHost', 'kIncrementHost', 'kDecrementHost', 'kRandomHost'
        self.target_proto_addr_mode = "fixed"
        self.target_proto_addr = 0x0
        self.target_proto_addr_count = 16
        self.target_proto_addr_mask = 0xffffff00

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kArpFieldNumber

        ost_arp = ost_p.Extensions[arp_pb2.arp]
        ost_arp.op_code                     = self.op_code

        if self.sender_hw_addr_mode == "increment":
            ost_arp.sender_hw_addr_mode     = ost_arp.kIncrement
        elif self.sender_hw_addr_mode == "decrement":
            ost_arp.sender_hw_addr_mode     = ost_arp.kDecrement
        else:
            ost_arp.sender_hw_addr_mode     = ost_arp.kFixed
        ost_arp.sender_hw_addr              = self.sender_hw_addr
        ost_arp.sender_hw_addr_count        = self.sender_hw_addr_count

        if self.sender_proto_addr_mode == "increment":
            ost_arp.sender_proto_addr_mode  = ost_arp.kIncrementHost
        elif self.sender_proto_addr_mode == "decrement":
            ost_arp.sender_proto_addr_mode  = ost_arp.kDecrementHost
        elif self.sender_proto_addr_mode == "random":
            ost_arp.sender_proto_addr_mode  = ost_arp.kRandomHost
        else:
            ost_arp.sender_proto_addr_mode  = ost_arp.kFixedHost

        if self.target_hw_addr_mode == "increment":
            ost_arp.target_hw_addr_mode     = ost_arp.kIncrement
        elif self.target_hw_addr_mode == "decrement":
            ost_arp.target_hw_addr_mode     = ost_arp.kDecrement
        else:
            ost_arp.target_hw_addr_mode = ost_arp.kFixed
        ost_arp.target_hw_addr              = self.target_hw_addr
        ost_arp.target_hw_addr_count        = self.target_hw_addr_count

        if self.target_proto_addr_mode == "increment":
            ost_arp.target_proto_addr_mode  = ost_arp.kIncrementHost
        elif self.target_proto_addr_mode == "decrement":
            ost_arp.target_proto_addr_mode  = ost_arp.kDecrementHost
        elif self.target_proto_addr_mode == "random":
            ost_arp.target_proto_addr_mode  = ost_arp.kRandomHost
        else:
            ost_arp.target_proto_addr_mode  = ost_arp.kFixedHost
        ost_arp.target_proto_addr           = self.target_proto_addr
        ost_arp.target_proto_addr_count     = self.target_proto_addr_count
        ost_arp.target_proto_addr_mask      = self.target_proto_addr_mask

class TrafficProtocolPayload:
    """Class that holds the Payload Protocol attributes

    Attributes:
        pattern_mode(str):  "fixed" (default), "increment", "decrement", "random"
        pattern(int):       0x0 (default)  Byte patterns
    """

    def __init__(self):
        ## value can be fixed, increment, decrement, random
        ## 'e_dp_fixed_word', 'e_dp_inc_byte', 'e_dp_dec_byte', 'e_dp_random'
        self.pattern_mode = "fixed"
        self.pattern = 0x0

    def apply_protocol(self, ost_s):
        """
        Converts internal data structure to ostinato data object

        Args:
            ost_s:   Ostinato stream object to have the protocol added
        """

        ost_p = ost_s.protocol.add()
        ost_p.protocol_id.id = ost_pb.Protocol.kPayloadFieldNumber

        ost_payload = ost_p.Extensions[payload_pb2.payload]
        ost_payload.pattern = self.pattern

        if self.pattern_mode == "increment":
            ost_payload.pattern_mode = ost_p.Extensions[payload_pb2.payload].e_dp_inc_byte
        elif self.pattern_mode == "decrement":
            ost_payload.pattern_mode = ost_p.Extensions[payload_pb2.payload].e_dp_dec_byte
        elif self.pattern_mode == "random":
            ost_payload.pattern_mode = ost_p.Extensions[payload_pb2.payload].e_dp_random
        else:
            ost_payload.pattern_mode = ost_p.Extensions[payload_pb2.payload].e_dp_fixed_word
