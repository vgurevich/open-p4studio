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

import traffic_protocols
from traffic_protocols import convert_mac_string_to_int, convert_mac_int_to_string
from traffic_protocols import convert_ipv4_string_to_int, convert_ipv4_int_to_string
from traffic_protocols import convert_ipv6_string_to_int, convert_ipv6_int_to_string

class TrafficStream:
    """Class used hold traffic stream information.

    Each TrafficStream instance holds protocol_list that conatins TrafficProtocol object.
    Also each instance has rx_all_ports that has list of ReceivePort objects.

    User can modify the Traffic Stream attributes directly, and all config for a given
    traffic stream will be pushed to Drone server once apply_stream is called.
    """
    def __init__(self, port_id, stream_id, rx_all_ports={}, rx_any_ports=[], rx_any_ports_packets=0):
        """
        Args:
            port_id(int):           ostinato port id (not the port name)
            stream_id(int):         stream id to be used
            rx_all_ports(dict):     expected {(drone server, receive port name):number of packets}
            rx_any_ports(list):     expected [(server, rx port), (server, rx port)]
            rx_any_ports_packets(int): expected total number of packets to be received on rx_any_ports

        Attributes:
            port_id(int):           ostinato port id

            rx_all_ports(dict):     list of ReceivePort for this stream
            rx_any_ports(list):
            rx_any_ports_packets(int):
            stream_id(int):         stream id
            is_enabled(bool):       stream is enabled if True(default), disabled if False
            control_mode(str):      "fixed" (Ostinato does not support continuous mode yet)
            stream_control(str):    "next"(default), "first", "stop"
            frame_len_mode(str):    "fixed"(default), "increment", "decrement", "random"
            frame_len(int):         frame length / packet size (default = 128)
            frame_len_min(int):     frame length minimum (use with non fixed mode, default=64)
            frame_len_max(int):     frame length maximum (use with non fixed mode, default=1518)
            control_unit(str):      "packets"(default), "bursts"
            num_packets(int):       number of packets to be sent (default = 10)
            packets_per_sec(int):   number of packets per second (default = 1)
            num_bursts(int):        number of bursts (default = 1)
            packets_per_burst(int): number of packets per bursts (default = 10)

            protocol_list(list):    list of dictionary of protocols object used in this stream

            ost_stream:             Ostinato stream object
        """

        self.port_id = port_id
        self.stream_id = stream_id
        self.rx_all_ports = {}
        self.rx_any_ports = []
        self.rx_any_ports_packets = rx_any_ports_packets

        for rx_port_tuple, expected_num_packet in rx_all_ports.iteritems():
            self.rx_all_ports[rx_port_tuple] = ReceivePort(rx_port_tuple, expected_num_packet)

        for rx_port_tuple in rx_any_ports:
            self.rx_any_ports.append(ReceivePort(rx_port_tuple))

        self.is_enabled = True
        ## value can be "fixed" or "continuous", currently GUI only supports "fixed"
        ## Ostinato constant variables: mode = 'e_sm_continuous', 'e_sm_fixed'
        self.control_mode = "fixed"

        ## value can be "next", "first", "stop"
        ## Ostinato constant variables: 'e_nw_goto_next', 'e_nw_goto_id', 'e_nw_stop'
        self.stream_control = "next"

        ## value can be "fixed", "increment", "decrement", "random"
        ## Ostinato constant variables: len_mode = 'e_fl_fixed', 'e_fl_inc', 'e_fl_dec', 'e_fl_random'
        self.frame_len_mode = "fixed"
        self.frame_len = 128
        self.frame_len_min = 64
        self.frame_len_max = 1518

        ## value can be "packets" or "bursts"
        ## Ostinato constant variables: unit = 'e_su_bursts', 'e_su_packets'
        self.control_unit = "packets"
        self.num_packets = 10
        self.packets_per_sec = 1
        self.num_bursts = 1
        self.packets_per_burst = 10
        self.bursts_per_sec = 1

        self.protocol_list = []

        self.ost_stream = ost_pb.StreamIdList()
        self.ost_stream.port_id.id = self.port_id
        self.ost_stream.stream_id.add().id = self.stream_id

    def set_packet_control(self,
                           packet_mode="packets",
                           num_packets=5,
                           packets_per_sec=10,
                           num_bursts=1,
                           packets_per_burst=5):
        if packet_mode=="packets":
            self.set_packet_mode(num_packets=num_packets,
                            packets_per_sec=packets_per_sec)
        else:
            self.set_burst_mode(num_bursts=num_bursts,
                           packets_per_burst=packets_per_burst)

    def set_burst_mode(self, num_bursts, packets_per_burst):
        self.control_unit = "bursts"
        self.num_bursts = num_bursts
        self.packets_per_burst = packets_per_burst

    def set_packet_mode(self, num_packets, packets_per_sec):
        self.control_unit = "packets"
        self.num_packets = num_packets
        self.packets_per_sec = packets_per_sec

    def set_frame_len_mode(self,
                           frame_len_mode="fixed",
                           frame_len_min=64,
                           frame_len_max=1518,
                           frame_len=100):
        if frame_len_mode=="fixed":
            self.set_fixed_frame_len(frame_len=frame_len)
        elif frame_len_mode=="increment":
            self.set_increment_frame_len(frame_len_min=frame_len_min,
                                        frame_len_max=frame_len_max)
        elif frame_len_mode=="decrement":
            self.set_decrement_frame_len(frame_len_min=frame_len_min,
                                        frame_len_max=frame_len_max)
        else:
            self.set_random_frame_len

    def set_fixed_frame_len(self, frame_len):
        self.frame_len_mode = "fixed"
        self.frame_len = frame_len

    def set_increment_frame_len(self, frame_len_min, frame_len_max):
        self.frame_len_mode = "increment"
        self.frame_len_min = frame_len_min
        self.frame_len_max = frame_len_max

    def set_decrement_frame_len(self, frame_len_min, frame_len_max):
        self.frame_len_mode = "decrement"
        self.frame_len_min = frame_len_min
        self.frame_len_max = frame_len_max

    def set_random_frame_len(self):
        self.frame_len_mode = "random"

    def set_display_filter_all_ports(self, port_tuple, filter_string):
        if port_tuple in self.rx_all_ports:
            self.rx_all_ports[port_tuple].set_display_filter(filter_string)
        else:
            print "Receive port {0} not found in stream id {1}".format(port_tuple, self.stream_id)
            assert()

    def append_display_filter_all_ports(self, port_tuple, filter_string):
        if port_tuple in self.rx_all_ports:
            self.rx_all_ports[port_tuple].append_display_filter(filter_string)
        else:
            print "Receive port {0} not found in stream id {1}".format(port_tuple, self.stream_id)
            assert()

    def set_display_filter_any_ports(self, filter_string):
         for receive_port in self.rx_any_ports:
             receive_port.set_display_filter(filter_string)

    def protocol_search(self, protocol):
        for item in self.protocol_list:
            if item.has_key(protocol):
                return item[protocol]
        return None

    def add_protocol(self, protocol, inner_vlan_id=1, outer_vlan_id=1, vni=0):
        """
        Add traffic protocol to this stream

        Args:
            protocol(str):  protocol name such as mac, vlan, eth2, ipv4, etc to be added to this stream
            inner_vlan_id(int): vlan tag (default=1) used for dot1q, or used for inner vlan tag for stacked vlan
            outer_vlan_id(int): outer vlan tag (default=1) used for stacked vlan
        """

        ## L1 ##
        if protocol == "mac":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolMac()})
        ## Dot1q ##
        elif protocol == "vlan":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolVlan(inner_vlan_id)})
        elif protocol == "stacked_vlan":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolStackedVlan(outer_vlan_id, inner_vlan_id)})
        ## Vxlan ##
        elif protocol == "vxlan":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolVxlan(vni)})
        ## L2 ##
        elif protocol == "eth2":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolEth2()})
        ## L3 ##
        elif protocol == "ipv4":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolIpv4()})
        elif protocol == "ipv6":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolIpv6()})
        elif protocol == "arp":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolArp()})
        ## L4 ##
        elif protocol == "tcp":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolTcp()})
        elif protocol == "udp":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolUdp()})
        elif protocol == "icmp":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolIcmp()})
        elif protocol == "igmp":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolIgmp()})
        ## Payload ##
        elif protocol == "payload":
            self.protocol_list.append({protocol : traffic_protocols.TrafficProtocolPayload()})
        else:
            pass

    def add_template_ipv4(self,
                          src_mac='00:01:02:03:04:05',
                          dst_mac='00:06:07:08:09:0a',
                          src_mac_mode="fixed",
                          src_mac_count=1,
                          src_mac_step=1,
                          dst_mac_mode="fixed",
                          dst_mac_count=1,
                          dst_mac_step=1,
                          src_ip='192.168.0.1',
                          dst_ip='192.168.0.2',
                          src_ip_mode="fixed",
                          src_ip_count=1,
                          src_ip_step=1,
                          dst_ip_mode="fixed",
                          dst_ip_count=1,
                          dst_ip_step=1,
                          ttl=64,
                          vlan_enable=False,
                          vlan_id = 0):
        """
        Sample IPv4 protocol add method

        Args:
            src_mac(str):   source mac
            dst_mac(str):   destinaton mac
            src_ip(str):    source ip
            dst_ip(str):    destination ip
            ttl(int):       ttl
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):   vlan tag (default = 0)
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv4')
        self.protocol_list[-1]['ipv4'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv4'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.protocol_list[-1]['ipv4'].set_ttl(ttl)
        self.add_protocol('payload')

    def add_template_ipv6(self,
                          src_mac='00:01:02:03:04:05',
                          dst_mac='00:06:07:08:09:0a',
                          src_mac_mode="fixed",
                          src_mac_count=1,
                          src_mac_step=1,
                          dst_mac_mode="fixed",
                          dst_mac_count=1,
                          dst_mac_step=1,
                          src_ip='2001:db8:85a3::8a2e:370:7334',
                          dst_ip='2001:db8:85a3::8a2e:370:7335',
                          src_ip_mode="fixed",
                          src_ip_count=1,
                          src_ip_step=1,
                          dst_ip_mode="fixed",
                          dst_ip_count=1,
                          dst_ip_step=1,
                          hlim=64,
                          vlan_enable=False,
                          vlan_id = 0):
        """
        Sample IPv6 protocol add method

        Args:
            src_mac(str):   source mac
            dst_mac(str):   destinaton mac
            src_ip(str):    source ipv6
            dst_ip(str):    destination ipv6
            hlim(int):      hop limit
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):   vlan tag (default = 0)
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv6')
        self.protocol_list[-1]['ipv6'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv6'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.protocol_list[-1]['ipv6'].set_hlim(hlim)
        self.add_protocol('payload')

    def add_template_tcpv4(self,
                           src_mac='00:01:02:03:04:05',
                           dst_mac='00:06:07:08:09:0a',
                           src_mac_mode="fixed",
                           src_mac_count=1,
                           src_mac_step=1,
                           dst_mac_mode="fixed",
                           dst_mac_count=1,
                           dst_mac_step=1,
                           src_ip='192.168.0.1',
                           dst_ip='192.168.0.2',
                           src_ip_mode="fixed",
                           src_ip_count=1,
                           src_ip_step=1,
                           dst_ip_mode="fixed",
                           dst_ip_count=1,
                           dst_ip_step=1,
                           src_port=1234,
                           dst_port=80,
                           ttl=64,
                           vlan_enable=False,
                           vlan_id = 0):
        """
        Sample TCPv4 protocol add method

        Args:
            src_mac(str):   source mac
            dst_mac(str):   destinaton mac
            src_ip(str):    source ip
            dst_ip(str):    destination ip
            src_port(int):  source tcp port
            dst_port(int):  destination tcp port
            ttl(int):       ttl
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):   vlan tag (default = 0)
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv4')
        self.protocol_list[-1]['ipv4'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv4'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.protocol_list[-1]['ipv4'].set_ttl(ttl)
        self.add_protocol('tcp')
        self.protocol_list[-1]['tcp'].is_override_src_port = True
        self.protocol_list[-1]['tcp'].src_port = src_port
        self.protocol_list[-1]['tcp'].is_override_dst_port = True
        self.protocol_list[-1]['tcp'].dst_port = dst_port
        self.add_protocol('payload')

    def add_template_tcpv6(self,
                           src_mac='00:01:02:03:04:05',
                           dst_mac='00:06:07:08:09:0a',
                           src_mac_mode="fixed",
                           src_mac_count=1,
                           src_mac_step=1,
                           dst_mac_mode="fixed",
                           dst_mac_count=1,
                           dst_mac_step=1,
                           src_ip='2001:db8:85a3::8a2e:370:7334',
                           dst_ip='2001:db8:85a3::8a2e:370:7335',
                           src_ip_mode="fixed",
                           src_ip_count=1,
                           src_ip_step=1,
                           dst_ip_mode="fixed",
                           dst_ip_count=1,
                           dst_ip_step=1,
                           src_port=1234,
                           dst_port=80,
                           hlim=64,
                           vlan_enable=False,
                           vlan_id = 0):
        """
        Sample TCPv6 protocol add method

        Args:
            src_mac(str):   source mac
            dst_mac(str):   destinaton mac
            src_ip(str):    source ip
            dst_ip(str):    destination ip
            src_port(int):  source tcp port
            dst_port(int):  destination tcp port
            hlim(int):      hop-limit
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):   vlan tag (default = 0)
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv6')
        self.protocol_list[-1]['ipv6'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv6'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.protocol_list[-1]['ipv6'].set_hlim(hlim)
        self.add_protocol('tcp')
        self.protocol_list[-1]['tcp'].is_override_src_port = True
        self.protocol_list[-1]['tcp'].src_port = src_port
        self.protocol_list[-1]['tcp'].is_override_dst_port = True
        self.protocol_list[-1]['tcp'].dst_port = dst_port
        self.add_protocol('payload')


    def add_template_udpv4(self,
                           src_mac='00:01:02:03:04:05',
                           dst_mac='00:06:07:08:09:0a',
                           src_mac_mode="fixed",
                           src_mac_count=1,
                           src_mac_step=1,
                           dst_mac_mode="fixed",
                           dst_mac_count=1,
                           dst_mac_step=1,
                           src_ip='192.168.0.1',
                           dst_ip='192.168.0.2',
                           src_ip_mode="fixed",
                           src_ip_count=1,
                           src_ip_step=1,
                           dst_ip_mode="fixed",
                           dst_ip_count=1,
                           dst_ip_step=1,
                           src_port=1234,
                           dst_port=80,
                           ttl=64,
                           vlan_enable=False,
                           vlan_id = 0):
        """
        Sample UDPv4 protocol add method

        Args:
            src_mac(str):   source mac
            dst_mac(str):   destinaton mac
            src_ip(str):    source ip
            dst_ip(str):    destination ip
            src_port(int):  source udp port
            dst_port(int):  destination udp port
            ttl(int):       ttl
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):   vlan tag (default = 0)
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv4')
        self.protocol_list[-1]['ipv4'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv4'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.protocol_list[-1]['ipv4'].set_ttl(ttl)
        self.add_protocol('udp')
        self.protocol_list[-1]['udp'].is_override_src_port = True
        self.protocol_list[-1]['udp'].src_port = src_port
        self.protocol_list[-1]['udp'].is_override_dst_port = True
        self.protocol_list[-1]['udp'].dst_port = dst_port
        self.add_protocol('payload')

    def add_template_udpv6(self,
                           src_mac='00:01:02:03:04:05',
                           dst_mac='00:06:07:08:09:0a',
                           src_mac_mode="fixed",
                           src_mac_count=1,
                           src_mac_step=1,
                           dst_mac_mode="fixed",
                           dst_mac_count=1,
                           dst_mac_step=1,
                           src_ip='2001:db8:85a3::8a2e:370:7334',
                           dst_ip='2001:db8:85a3::8a2e:370:7335',
                           src_ip_mode="fixed",
                           src_ip_count=1,
                           src_ip_step=1,
                           dst_ip_mode="fixed",
                           dst_ip_count=1,
                           dst_ip_step=1,
                           src_port=1234,
                           dst_port=80,
                           hlim=64,
                           vlan_enable=False,
                           vlan_id = 0):
        """
        Sample UDPv4 protocol add method

        Args:
            src_mac(str):   source mac
            dst_mac(str):   destinaton mac
            src_ip(str):    source ip
            dst_ip(str):    destination ip
            src_port(int):  source udp port
            dst_port(int):  destination udp port
            hlim(int):      hop-limit
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):   vlan tag (default = 0)
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv6')
        self.protocol_list[-1]['ipv6'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv6'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.protocol_list[-1]['ipv6'].set_hlim(hlim)
        self.add_protocol('udp')
        self.protocol_list[-1]['udp'].is_override_src_port = True
        self.protocol_list[-1]['udp'].src_port = src_port
        self.protocol_list[-1]['udp'].is_override_dst_port = True
        self.protocol_list[-1]['udp'].dst_port = dst_port
        self.add_protocol('payload')

    def add_template_vxlan_header(self,
                           vxlan_vni = 0xaba,
                           src_mac = '00:06:07:08:09:0a',
                           dst_mac = '00:01:02:03:04:05',
                           src_mac_mode="fixed",
                           src_mac_count=1,
                           src_mac_step=1,
                           dst_mac_mode="fixed",
                           dst_mac_count=1,
                           dst_mac_step=1,
                           vlan_enable=False,
                           vlan_id = 0,
                           src_ip = '192.168.0.1',
                           dst_ip = '192.168.0.2',
                           src_ip_mode="fixed",
                           src_ip_count=1,
                           src_ip_step=1,
                           dst_ip_mode="fixed",
                           dst_ip_count=1,
                           dst_ip_step=1,
                           src_port = 1234,
                           dst_port = 4789,
                           udp_chksum = False):
        """
        Sample VxLAN protocol add header method, user needs to call
        other method to add to the inner frame, such as add_template_tcpv4

        Args:
            vxlan_vni(int):   VxLAN VNI
            src_mac(str):     source mac
            dst_mac(str):     destinaton mac
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):     vlan tag (default = 0)
            src_ip(str):      source ip
            dst_ip(str):      destination ip
            src_port(int):   source udp port (default = 1234)
            dst_port(int):   destination udp port (default = 4789 for vxlan)
            udp_chksum(bool): add udp checksum if True, checksum = 0 if False
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv4')
        self.protocol_list[-1]['ipv4'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv4'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.add_protocol('udp')
        self.protocol_list[-1]['udp'].is_override_src_port = True
        self.protocol_list[-1]['udp'].src_port = src_port
        self.protocol_list[-1]['udp'].is_override_dst_port = True
        self.protocol_list[-1]['udp'].dst_port = dst_port
        if udp_chksum == False:
            self.protocol_list[-1]['udp'].is_override_cksum = True
            self.protocol_list[-1]['udp'].cksum = 0
        self.add_protocol('vxlan', vni = vxlan_vni)


    def add_template_vxlanv6_header(self,
                           vxlan_vni = 0xaba,
                           src_mac = '00:06:07:08:09:0a',
                           dst_mac = '00:01:02:03:04:05',
                           src_mac_mode="fixed",
                           src_mac_count=1,
                           src_mac_step=1,
                           dst_mac_mode="fixed",
                           dst_mac_count=1,
                           dst_mac_step=1,
                           vlan_enable=False,
                           vlan_id = 0,
                           src_ip = '1::2',
                           dst_ip = '3::4',
                           src_ip_mode="fixed",
                           src_ip_count=1,
                           src_ip_step=1,
                           dst_ip_mode="fixed",
                           dst_ip_count=1,
                           dst_ip_step=1,
                           src_port = 1234,
                           dst_port = 4789,
                           udp_chksum = False):
        """
        Sample VxLAN protocol add header method, user needs to call
        other method to add to the inner frame, such as add_template_tcpv6

        Args:
            vxlan_vni(int):   VxLAN VNI
            src_mac(str):     source mac
            dst_mac(str):     destinaton mac
            vlan_enable(bool):  Add dot1q tag if True
            vlan_id(int):     vlan tag (default = 0)
            src_ip(str):      source ip
            dst_ip(str):      destination ip
            src_port(int):   source udp port (default = 1234)
            dst_port(int):   destination udp port (default = 4789 for vxlan)
            udp_chksum(bool): add udp checksum if True, checksum = 0 if False
        """

        self.add_protocol('mac')
        self.protocol_list[-1]['mac'].set_src_mac(mac=src_mac,
                                                  mac_mode=src_mac_mode,
                                                  mac_count=src_mac_count,
                                                  mac_step=src_mac_step)
        self.protocol_list[-1]['mac'].set_dst_mac(mac=dst_mac,
                                                  mac_mode=dst_mac_mode,
                                                  mac_count=dst_mac_count,
                                                  mac_step=dst_mac_step)
        if vlan_enable:
            self.add_protocol('vlan', vlan_id)
        self.add_protocol('eth2')
        self.add_protocol('ipv6')
        self.protocol_list[-1]['ipv6'].set_src_ip(ip=src_ip,
                                                  ip_mode=src_ip_mode,
                                                  ip_count=src_ip_count,
                                                  ip_step=src_ip_step)
        self.protocol_list[-1]['ipv6'].set_dst_ip(ip=dst_ip,
                                                  ip_mode=dst_ip_mode,
                                                  ip_count=dst_ip_count,
                                                  ip_step=dst_ip_step)
        self.add_protocol('udp')
        self.protocol_list[-1]['udp'].is_override_src_port = True
        self.protocol_list[-1]['udp'].src_port = src_port
        self.protocol_list[-1]['udp'].is_override_dst_port = True
        self.protocol_list[-1]['udp'].dst_port = dst_port
        if udp_chksum == False:
            self.protocol_list[-1]['udp'].is_override_cksum = True
            self.protocol_list[-1]['udp'].cksum = 0
        self.add_protocol('vxlan', vni = vxlan_vni)


    def apply_stream(self, server):
        """
        Apply / pushes the stream profile to the Drone server
        Filter strings is also getting updated in the internal data structure

        Args:
            server:   TrafficTool object to apply config to Drone server
        """
        ## clean up stream config on server first
        try:
         server.deleteStream(self.ost_stream)
        except:
         pass

        ## add the new stream
        server.addStream(self.ost_stream)

        ost_port = ost_pb.PortIdList()
        ost_port.port_id.add().id = self.port_id

        ost_stream_cfg = ost_pb.StreamConfigList()
        ost_stream_cfg.port_id.CopyFrom(ost_port.port_id[0])

        ost_s = ost_stream_cfg.stream.add()
        ost_s.stream_id.id = self.ost_stream.stream_id[0].id

        ost_s.core.is_enabled = self.is_enabled

        if self.frame_len_mode == "random":
            ost_s.core.len_mode = ost_s.core.e_fl_random
        elif self.frame_len_mode == "increment":
            ost_s.core.len_mode = ost_s.core.e_fl_inc
        elif self.frame_len_mode == "decrement":
            ost_s.core.len_mode = ost_s.core.e_fl_dec
        else:
            ost_s.core.len_mode = ost_s.core.e_fl_fixed
        ost_s.core.frame_len     = self.frame_len
        ost_s.core.frame_len_min = self.frame_len_min
        ost_s.core.frame_len_max = self.frame_len_max

        if self.stream_control == "first":
            ost_s.control.next = ost_s.control.e_nw_goto_id
        elif self.stream_control == "stop":
            ost_s.control.next = ost_s.control.e_nw_stop
        else:
            ost_s.control.next = ost_s.control.e_nw_goto_next

        if self.control_unit == "bursts":
            ost_s.control.unit = ost_s.control.e_su_bursts
        else:
            ost_s.control.unit = ost_s.control.e_su_packets
        ost_s.control.packets_per_sec   = self.packets_per_sec
        ost_s.control.num_packets       = self.num_packets
        ost_s.control.num_bursts        = self.num_bursts
        ost_s.control.packets_per_burst = self.packets_per_burst
        ost_s.control.bursts_per_sec    = self.bursts_per_sec

        for protocol in self.protocol_list:
            if protocol.has_key('mac'):
                protocol['mac'].apply_protocol(ost_s)

            if protocol.has_key('vlan'):
                protocol['vlan'].apply_protocol(ost_s)

            if protocol.has_key('vxlan'):
                protocol['vxlan'].apply_protocol(ost_s)

            if protocol.has_key('stacked_vlan'):
                protocol['stacked_vlan'].apply_protocol(ost_s)

            if protocol.has_key('eth2'):
                protocol['eth2'].apply_protocol(ost_s)

            if protocol.has_key('ipv4'):
                protocol['ipv4'].apply_protocol(ost_s)

            if protocol.has_key('ipv6'):
                protocol['ipv6'].apply_protocol(ost_s)

            if protocol.has_key('arp'):
                protocol['arp'].apply_protocol(ost_s)

            if protocol.has_key('tcp'):
                protocol['tcp'].apply_protocol(ost_s)

            if protocol.has_key('udp'):
                protocol['udp'].apply_protocol(ost_s)

            if protocol.has_key('icmp'):
                protocol['icmp'].apply_protocol(ost_s)

            if protocol.has_key('igmp'):
                protocol['igmp'].apply_protocol(ost_s)

            if protocol.has_key('payload'):
                protocol['payload'].apply_protocol(ost_s)

        server.modifyStream(ost_stream_cfg)

        ## Update the display filter used for rx port
        for rx_port_tuple in self.rx_all_ports:
            filter_string = ""

            if self.rx_all_ports[rx_port_tuple].display_filter_string == "":
                pass
                if self.protocol_search('mac') != None:
                    if self.protocol_search('ipv4') == None and self.protocol_search('ipv6') == None:
                        ## Pure L2 frame, no L3 header.  Can default add src and dst mac as filter
                        src_mac_string = convert_mac_int_to_string(self.protocol_search('mac').src_mac)
                        dst_mac_string = convert_mac_int_to_string(self.protocol_search('mac').dst_mac)
                        if filter_string != "":
                            filter_string += " and eth.src=={0} and eth.dst=={1}".format(src_mac_string, dst_mac_string)
                        else:
                            filter_string += "eth.src=={0} and eth.dst=={1}".format(src_mac_string, dst_mac_string)

                if self.protocol_search('ipv4') != None:
                    src_ip_string = convert_ipv4_int_to_string(self.protocol_search('ipv4').src_ip)
                    dst_ip_string = convert_ipv4_int_to_string(self.protocol_search('ipv4').dst_ip)
                    if filter_string != "":
                        filter_string += " and ip.src=={0} and ip.dst=={1}".format(src_ip_string, dst_ip_string)
                    else:
                        filter_string += "ip.src=={0} and ip.dst=={1}".format(src_ip_string, dst_ip_string)

                if self.protocol_search('ipv6') != None:
                    src_ipv6_string = convert_ipv6_int_to_string(self.protocol_search('ipv6').src_addr_hi, self.protocol_search('ipv6').src_addr_lo)
                    dst_ipv6_string = convert_ipv6_int_to_string(self.protocol_search('ipv6').dst_addr_hi, self.protocol_search('ipv6').dst_addr_lo)
                    if filter_string != "":
                        filter_string += " and ipv6.src=={0} and ipv6.dst=={1}".format(src_ipv6_string, dst_ipv6_string)
                    else:
                        filter_string += "ipv6.src=={0} and ipv6.dst=={1}".format(src_ipv6_string, dst_ipv6_string)
            else:
                filter_string = self.rx_all_ports[rx_port_tuple].display_filter_string

            if self.rx_all_ports[rx_port_tuple].append_filter_string != "":
                filter_string += " and {0}".format(self.rx_all_ports[rx_port_tuple].append_filter_string)

            self.rx_all_ports[rx_port_tuple].display_filter_string = filter_string

class ReceivePort:
    """Class used to define the properties of the expected traffic receive port

    Each ReceivePort instance holds the port name, expected number of packets, and filter
    used in Wireshark.  The filter can be default of src mac and src / dst ipv4/v6, or
    can be set manually, or can be appended on top of the default.
    """

    def __init__(self, port_tuple, expected_num_packet=0):
        self.port_tuple = port_tuple
        self.server = port_tuple[0]
        self.port_name = port_tuple[1]
        self.expected_num_packet = expected_num_packet
        self.display_filter_string = ""
        self.append_filter_string = ""

    def append_display_filter(self, filter_string):
        self.append_filter_string += " " + filter_string

    def set_display_filter(self, filter_string):
        self.display_filter_string = filter_string

    def verify_traffic(self, pcap_file):
        """
        Verify if the given pcap_file, apply with either default filter, or user filter in
        Wireshark, has the same actual packet count as the expected packet count.

        Args:
            pcap_file:   pcap_file for the captured traffic

        Returns:
            boolean:     True if actual number of packets is same as expected.  False otherwise
        """

        tshark_string = 'tshark -r {0} -Y \"{1}\"| wc -l'.format(pcap_file, self.display_filter_string)
        print "Verifying traffic with tshark: {0}".format(tshark_string)

        shell_exec = subprocess.Popen([tshark_string], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        (out, err) = shell_exec.communicate()
        if shell_exec.returncode != 0:
            print "tshark not terminatedly normally with return code {0}".shell_exec.returncode
            assert()
        actual_num_packet = int(out.split("\n")[0])
        print "Expected packet = {0}, Actual received = {1}".format(self.expected_num_packet, actual_num_packet)

        return self.expected_num_packet == actual_num_packet

    def get_traffic_count(self, pcap_file):
        tshark_string = 'tshark -r {0} -Y \"{1}\"| wc -l'.format(pcap_file, self.display_filter_string)
        shell_exec = subprocess.Popen([tshark_string], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        (out, err) = shell_exec.communicate()
        if shell_exec.returncode != 0:
            print "tshark not terminatedly normally with return code {0}".shell_exec.returncode
            assert()
        actual_num_packet = int(out.split("\n")[0])

        return int(actual_num_packet)
