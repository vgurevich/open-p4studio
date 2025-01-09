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


import logging
import os
import sys
import time
import subprocess

this_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(os.path.join(this_dir,
                             '../install//lib/python2.7/site-packages'))
import ptf
from ostinato.core import ost_pb, DroneProxy
from traffic_protocols import convert_mac_string_to_int, convert_mac_int_to_string
from traffic_protocols import convert_ipv4_string_to_int, convert_ipv4_int_to_string
from traffic_protocols import convert_ipv6_string_to_int, convert_ipv6_int_to_string
import traffic_streams

"""
traffic_utils contains list of classes and methods to use Ostinato utility as traffic
generator, as well as using tshark with display filter for traffic verfication
"""

class TrafficTool:

    def __init__(self):
        self.traffic_server_list = dict()
        if not os.path.exists('traffic_pcap'):
            os.makedirs('traffic_pcap')

    def connect(self, host_name, port_number=7878):
        if host_name not in self.traffic_server_list:
            self.traffic_server_list[host_name] = TrafficServer(host_name, port_number)
            return self.traffic_server_list[host_name]

    def disconnect(self, host_name='all'):

        if host_name == 'all':
            for host in self.traffic_server_list:
                self.traffic_server_list[host].disconnect()

        elif host_name in self.traffic_server_list:
            self.traffic_server_list[host_name].disconnect()
            del self.traffic_server_list[host_name]

        else:
            pass

    def start_transmit(self, port_tuple_list='all'):
        """
        Start transmit in Ostinato on the Drone server for a given port_list

        Args:
            port_tuple_list:  (server,Port name) tuple list to start transmit
        """
        if port_tuple_list == 'all':
            for host in self.traffic_server_list:
                self.traffic_server_list[host].start_transmit('all')
        else:
            for port_tuple in port_tuple_list:
                server_ip = port_tuple[0]
                port = port_tuple[1]
                self.traffic_server_list[server_ip].start_transmit([port])

    def stop_transmit(self, port_tuple_list='all'):
        """
        Stop transmit in Ostinato on the Drone server for a given port_list

        Args:
            port_tuple_list:  (server,Port name) tuple list to stop transmit
        """

        if port_tuple_list == 'all':
            for host in self.traffic_server_list:
                self.traffic_server_list[host].stop_transmit('all')
        else:
            for port_tuple in port_tuple_list:
                server_ip = port_tuple[0]
                port = port_tuple[1]
                self.traffic_server_list[server_ip].stop_transmit([port])

    def start_capture(self, port_tuple_list='all'):
        """
        Start capture in Ostinato on the Drone server for a given port_list

        Args:
            port_tuple_list:  (server,Port name) tuple list to start capture
        """
        if port_tuple_list == 'all':
            for host in self.traffic_server_list:
                self.traffic_server_list[host].start_capture('all')
        else:
            for port_tuple in port_tuple_list:
                host = port_tuple[0]
                port = port_tuple[1]
                self.traffic_server_list[host].start_capture([port])

    def stop_capture(self, port_tuple_list='all'):
        """
        Stop capture in Ostinato on the Drone server for a given port_list

        Args:
            port_tuple_list:  (server,Port name) tuple list to stop capture
        """

        if port_tuple_list == 'all':
            for host in self.traffic_server_list:
                self.traffic_server_list[host].stop_capture('all')
        else:
            for port_tuple in port_tuple_list:
                host = port_tuple[0]
                port = port_tuple[1]
                self.traffic_server_list[host].stop_capture([port])

    def send_traffic(self, port_tuple_list='all'):
        self.start_capture(port_tuple_list)
        self.start_transmit(port_tuple_list)

    def stop_traffic(self, port_tuple_list='all'):
        self.stop_transmit(port_tuple_list)
        self.stop_capture(port_tuple_list)

    def verify_traffic(self):
        """
        Verify if captured receive packets are as expected as defined in stream's rx_all_ports
        Internally loop through each TrafficPort in tx_port_list
        loop through each Streams in TrafficPort
        and finally loops through each ReceivePort in the rx_all_ports of each stream
        calls the RecievePort method verify_traffic
        """
        for host in self.traffic_server_list:
            self.traffic_server_list[host].verify_traffic()

class TrafficServer:
    """Class used to control Ostinato traffic generator, stores data used in each instance of Drone

    Each TrafficServer instance holds a list of port objects that can be traversed,
    configured, and take action upon.

    Attributes that has ost_ prefix are used specifically to hold the ostinado attribute,
    object, or data structure.

    Each Traffic, Port, Stream, Protocol class will most likely have two types of
    data structure.  One used for storing the user config during script setup.
    The other (with ost_ prefix) is used for storing / applying ostinato config during
    traffic stream / config apply.
    """

    def __init__(self, host_name, port_number=7878):
        """
        Args:
            host_name (string): hostname used for Drone connection.  Can be ip address also.
            port_number (int): port number used for Drone.  Default is 7878
        """

        #: bool to turn on / off print output
        self.log = True
        self.host = host_name
        self.port = port_number
        #: server is of DroneProxy class, used functions related to Drone / Ostinato communication
        self.server = DroneProxy(self.host, self.port)
        #: tx_port_list dict stores the TrafficPort objects, with ptf port_id as the key
        self.tx_port_list = dict()
        #: ptf_toost_port_map['ptf port_id'] stores the ptf port_id to Ostinato port id mapping
        self.ptf_to_ost_port_map = dict()
        #: ost_to_ptf_port_map['Ostinato port id'] stores the ostinato port id to ptf port_id mapping
        self.ost_to_ptf_port_map = dict()

        ## Connect to Drone
        self.connect()

        ## Populate mapping of ptf veth to ptf port_id
        self.ptf_veth_to_port_id = dict()

        for port_id, ifname in ptf.config["port_map"].items():
            device, port = port_id
            self.ptf_veth_to_port_id[ifname] = port

        ## Populate mapping of ptf port_id to Ostinato port id
        port_config_list = self.get_port_config (self.get_port_id_list())
        for port_config in port_config_list.port:
            veth_name = port_config.name
            port_id = port_config.port_id.id
            if veth_name in self.ptf_veth_to_port_id:
                ptf_port_id = self.ptf_veth_to_port_id[veth_name]
                self.ptf_to_ost_port_map[ptf_port_id] = port_id
                self.ost_to_ptf_port_map[port_id] = ptf_port_id


    def connect(self):
        self.server.connect()
        if self.log:
            print "Connecting to Drone Proxy at {0}:{1}".format(self.host, self.port)

    def disconnect(self):
        self.server.disconnect()
        if self.log:
            print "Disconnecting from Drone Proxy at {0}:{1}".format(self.host, self.port)


    def get_port_id_list(self):
        """
        Ostinato returns the port id of a given Drone instance in the following format:
        port_id_list[index].port_id.id

        Sample:
            port_id {
                id: 0
            }
            port_id {
                id: 1
            }
        """

        return self.server.getPortIdList()

    def get_port_config(self, port_id_list):
        """
        Ostinato returns the Drone port config info in the following format:
        port[index].port_id.id
        port[index].name

        Sample:
            port {
              port_id {
                id: 64
              }
              name: "veth63"
              notes: "<b>Limitation(s)</b><ul><li>Non Promiscuous Mode</li></ul>"
              is_enabled: true
              is_exclusive_control: false
              transmit_mode: 0
            }
        Args:
            port_id_list (list of port_id object returned from get_port_id_list)
        """

        return self.server.getPortConfig(port_id_list)

    def get_stats(self, port_list):
        """
        Ostinato returns the Drone port statistics info in the following format:
        port_stats[index].port_id.id
        port_stats[index].<stats_field> where stats_field are:

        rx_bps
        rx_bytes
        rx_bytes_nic
        rx_drops
        rx_erros
        rx_fifo_errors
        rx_frame_errors
        rx_pkts
        rx_pkts_nic
        rx_pps

        tx_bps
        tx_bytes
        tx_bytes_nic
        tx_pkts
        tx_pkts_nic
        tx_pps

        Typical use case would be packet count, which is the rx_pkts and tx_pkts counters

        Args:
            port_list:  Port name list to have stats collected for
        """

        ost_port_list = ost_pb.PortIdList()
        for port in port_list:
            ost_port_id = self.ptf_to_ost_port_map[port]
            ost_port_list.port_id.add().id = int(ost_port_id)

        return self.server.getStats(ost_port_list)

    def clear_stats(self, port_list):
        """
        Clear the port stats in Ostinato on the Drone server for a given port_list

        Args:
            port_list:  Port name list to have stats cleared
        """

        ost_port_list = ost_pb.PortIdList()
        for port in port_list:
            ost_port_id = self.ptf_to_ost_port_map[port]
            ost_port_list.port_id.add().id = int(ost_port_id)

        self.server.clearStats(ost_port_list)

    def start_transmit(self, port_list='all'):
        """
        Start transmit in Ostinato on the Drone server for a given port_list

        Args:
            port_list:  Port name list to start transmit
        """

        if port_list == 'all':
            port_list = self.tx_port_list.keys()

        ost_port_list = ost_pb.PortIdList()
        for port in port_list:
            ost_port_id = self.ptf_to_ost_port_map[port]
            ost_port_list.port_id.add().id = int(ost_port_id)

        self.server.startTransmit(ost_port_list)

    def stop_transmit(self, port_list='all'):
        """
        Stop transmit in Ostinato on the Drone server for a given port_list

        Args:
            port_list:  Port name list to stop transmit
        """

        if port_list == 'all':
            port_list = self.tx_port_list.keys()

        ost_port_list = ost_pb.PortIdList()
        for port in port_list:
            ost_port_id = self.ptf_to_ost_port_map[port]
            ost_port_list.port_id.add().id = int(ost_port_id)

        self.server.stopTransmit(ost_port_list)

    def start_capture(self, port_list='all'):
        """
        Start capture in Ostinato on the Drone server for a given port_list

        Args:
            port_list:  Port name list to start capture
        """

        ost_port_list = ost_pb.PortIdList()

        if port_list == 'all':
            port_list = self.ptf_to_ost_port_map.keys()

        for port in port_list:
            ost_port_id = self.ptf_to_ost_port_map[port]
            ost_port_list.port_id.add().id = int(ost_port_id)

        self.server.startCapture(ost_port_list)

    def stop_capture(self, port_list):
        """
        Stop capture in Ostinato on the Drone server for a given port_list

        Args:
            port_list:  Port name list to stop capture
        """
        ost_port_list = ost_pb.PortIdList()

        if port_list == 'all':
            port_list = self.ptf_to_ost_port_map.keys()

        for port in port_list:
            ost_port_id = self.ptf_to_ost_port_map[port]
            ost_port_list.port_id.add().id = int(ost_port_id)

        self.server.stopCapture(ost_port_list)

        for port in port_list:
            buff = self.get_capture_buffer(port)
            self.save_capture_buffer(buff, "traffic_pcap/" + self.host + "_" + str(port) + ".pcap")

    def send_traffic(self, port_list='all'):
        self.start_capture(port_list)
        self.start_transmit(port_list)

    def stop_traffic(self, port_list='all'):
        self.stop_transmit(port_list)
        self.stop_capture(port_list)


    def get_capture_buffer(self, port):
        """
        Get capture buffer in Ostinato on the Drone server for a given port

        Args:
            port:  Port name to get capture buffer
        """
        ost_port_id = self.ptf_to_ost_port_map[port]
        ost_port = ost_pb.PortIdList()
        ost_port.port_id.add().id = int(ost_port_id)

        return self.server.getCaptureBuffer(ost_port.port_id[0])

    def save_capture_buffer(self, buffer, file_name):
        """
        Save capture buffer to a file

        Args:
            buffer:     capture buffer returned from get_capture_buffer
            file_name:  file name to be saved for the capture file
        """

        self.server.saveCaptureBuffer(buffer, file_name)

    def add_tx_port(self, port_name):
        """
        Add transmit port to the internal data structure.  Instantiate a TrafficPort
        object in a given port and save in tx_port_list

        Sample:
            traffic = TrafficServer('127.0.0.1')
            traffic.add_tx_port(1)

            traffic.tx_port_list[1] can access the TrafficPort methods and data

        Args:
            port_name:   port name to be added as part of transmit port list
        """

        ost_port_id = self.ptf_to_ost_port_map[port_name]

        if port_name not in self.tx_port_list:
            self.tx_port_list[port_name] = TrafficPort(ost_port_id, port_name)

    def del_tx_port(self, port_name):
        """
        Delete transmit port from the internal data structure

        Args:
            port_name:   port name to be deleted from the transmit port list
        """

        if port_name in self.tx_port_list:
            del self.tx_port_list[port_name]

    def set_transmit_interleaved(self, port_name):
        """
        Set interleaved transmit mode for a given traffic port. Traffic streams
        will be in continuous mode and not by number of packets or bursts

        Args:
            port_name:   port name to be deleted from the transmit port list
        """
        if port_name not in self.tx_port_list:
            self.add_tx_port(port_name)

        self.tx_port_list[port_name].set_interleaved(self.server)

    def set_transmit_sequential(self, port_name):
        """
        Set sequential transmit mode for a given traffic port

        Args:
            port_name:   port name to be deleted from the transmit port list
            transmit_mode: 'sequential' (default), or 'interleaved'
        """

        if port_name not in self.tx_port_list:
            self.add_tx_port(port_name)

        self.tx_port_list[port_name].set_sequential(self.server)

    def add_stream(self, port_name, stream_id, rx_all_ports={}, rx_any_ports=[], rx_any_ports_packets=0):

        """
        Add traffic stream for a given port, stream id, and expected receivers
        Calls TrafficPort method add_stream internally for a given tx port

        Args:
            port_name:          port name for the traffic stream to be added
            stream_id:          stream id for the traffic stream
            rx_all_ports:    dictionary of expected receivers and number of packets
        """

        ## Create tx port id if it has not been created previously
        if port_name not in self.tx_port_list:
            self.add_tx_port(port_name)

        ## add server ip if the ports did not come as a tuple (ip, port)
        if isinstance(rx_any_ports, list):
            if rx_any_ports:
                for index, rx_port in enumerate(rx_any_ports):
                    if not isinstance (rx_port, tuple):
                        rx_any_ports[index] = (self.host, rx_port)

        return self.tx_port_list[port_name].add_stream(server = self.server,
                                                       stream_id = stream_id,
                                                       rx_all_ports = rx_all_ports,
                                                       rx_any_ports = rx_any_ports,
                                                       rx_any_ports_packets = rx_any_ports_packets)

    def delete_stream(self, port_name, stream_id):
        """
        Delete traffic stream for a given port, stream id

        Args:
            port_name:          port name for the traffic stream to be added
            stream_id:          stream id for the traffic stream
        """

        if port_name in self.tx_port_list:
            self.tx_port_list[port_name].del_stream(self.server, stream_id)

    def delete_all_streams(self):
        """
        Delete all traffic stream from this TrafficServer instance
        Internall loop through all port in tx_port_list TrafficPort method of del_all_streams
        """

        for port in self.tx_port_list:
            self.tx_port_list[port].del_all_streams(self.server)

    def get_stream(self, port_name, stream_id):
        """
        Get the traffic stream object matching the port_name and stream_id
        Raise an error if stream does not exist

        Args:
            port_name:          port name for the traffic stream
            stream_id:          stream id for the traffic stream
        """

        try:
            return self.tx_port_list[port_name].stream_list[stream_id]
        except:
            print "Port {0}, stream id {1} not found".format(port_name, stream_id)
            assert()

    def apply_config(self):
        """
        Apply / push the traffic config from internal data structure to Drone server
        Internally loop through all port in tx_port_list and
        calls TrafficPort apply_streams method
        """

        for port in self.tx_port_list:
            self.tx_port_list[port].apply_streams(self.server)

    def verify_traffic(self):
        self.verify_rx_all_ports_traffic()
        self.verify_rx_any_ports_traffic()

    def verify_rx_all_ports_traffic(self):
        """
        Verify if captured receive packets are as expected as defined in stream's rx_all_ports
        Internally loop through each TrafficPort in tx_port_list
        loop through each Streams in TrafficPort
        and finally loops through each ReceivePort in the rx_all_ports of each stream
        calls the RecievePort method verify_traffic
        """
        for port in self.tx_port_list:
            for stream in self.tx_port_list[port].stream_list:
                for rx_port_tuple in self.tx_port_list[port].stream_list[stream].rx_all_ports:
                    print "\nVerifying traffic tx port {0}:{1} stream{2} -> rx port {3}:{4}\
                        ".format(self.host, port, stream, rx_port_tuple[0], rx_port_tuple[1])
                    receive_port = self.tx_port_list[port].stream_list[stream].rx_all_ports[rx_port_tuple]
                    if not receive_port.verify_traffic("traffic_pcap/" + str(rx_port_tuple[0]) + "_" + str(rx_port_tuple[1]) + ".pcap"):
                        print "Traffic FAILED  !!!!"
                        assert()

    def verify_rx_any_ports_traffic(self):
        for port in self.tx_port_list:
            for stream in self.tx_port_list[port].stream_list:
                if not self.tx_port_list[port].stream_list[stream].rx_any_ports:
                    break
                actual_packet_count = 0
                expected_packet_count = self.tx_port_list[port].stream_list[stream].rx_any_ports_packets
                print "\nVerifying traffic tx port {0}:{1} stream{2} -> rx port".format(self.host, port, stream)

                for receive_port in self.tx_port_list[port].stream_list[stream].rx_any_ports:
                    pcap_file_name = "traffic_pcap/" + str(receive_port.server) + "_" + str(receive_port.port_name) + ".pcap"
                    port_rx_count = receive_port.get_traffic_count(pcap_file_name)
                    actual_packet_count += port_rx_count
                    print "== rx port {0}:{1}, count={2}".format(receive_port.server, receive_port.port_name, port_rx_count)

                if actual_packet_count != expected_packet_count:
                    print "Traffic FAILED  !!!!"
                    print "Expected packet count: {0}, Actual packet count: {1}".format(expected_packet_count, actual_packet_count)
                    assert()

    def verify_traffic_on_port(self, port=1, filter_string="", expected_packet_count=0):
        pcap_file_name = "traffic_pcap/" + str(self.host) + "_" + str(port) + ".pcap"
        tshark_string = 'tshark -r {0} -Y \"{1}\"| wc -l'.format(pcap_file_name, filter_string)
        print "Verifying traffic with tshark: {0}".format(tshark_string)
        shell_exec = subprocess.Popen([tshark_string], stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        (out, err) = shell_exec.communicate()
        if shell_exec.returncode != 0:
            print "tshark not terminatedly normally with return code {0}".shell_exec.returncode
            assert()
        actual_packet_count = int(out.split("\n")[0])
        if actual_packet_count != expected_packet_count:
            print "Traffic FAILED !!!!"
            print "Expected packet count: {0}, Actual packet count: {1}".format(expected_packet_count, actual_packet_count)
            assert()

    def simple_ipv4_traffic (self,
                            stream_id=1,
                            source_traffic_port=1,
                            expected_traffic_tuple={},
                            expected_any_ports=[],
                            expected_any_ports_packets=0,
                            packet_mode="packets",
                            num_packets=5,
                            packets_per_sec=10,
                            num_bursts=1,
                            packets_per_burst=5,
                            frame_len_mode="fixed",
                            frame_len_min=64,
                            frame_len_max=1518,
                            frame_len=100,
                            src_mac='00:01:02:03:04:05',
                            dst_mac='00:06:07:08:09:0a',
                            src_mac_mode="fixed",
                            src_mac_count=1,
                            src_mac_step=1,
                            dst_mac_mode="fixed",
                            dst_mac_count=1,
                            dst_mac_step=1,
                            vlan_enable=False,
                            vlan_id=0,
                            src_ip='192.168.0.1',
                            dst_ip='192.168.0.2',
                            src_ip_mode="fixed",
                            src_ip_count=1,
                            src_ip_step=1,
                            dst_ip_mode="fixed",
                            dst_ip_count=1,
                            dst_ip_step=1,
                            ttl=64,
                            set_traffic_filter_any_ports="",
                            set_traffic_filter_all_ports="",
                            append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_ipv4(src_mac=src_mac,
                                dst_mac=dst_mac,
                                src_mac_mode=src_mac_mode,
                                src_mac_count=src_mac_count,
                                src_mac_step=src_mac_step,
                                dst_mac_mode=dst_mac_mode,
                                dst_mac_count=dst_mac_count,
                                dst_mac_step=dst_mac_step,
                                src_ip=src_ip,
                                dst_ip=dst_ip,
                                src_ip_mode=src_ip_mode,
                                src_ip_count=src_ip_count,
                                src_ip_step=src_ip_step,
                                dst_ip_mode=dst_ip_mode,
                                dst_ip_count=dst_ip_count,
                                dst_ip_step=dst_ip_step,
                                ttl=ttl,
                                vlan_enable=vlan_enable,
                                vlan_id=vlan_id)
            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)
            return s

    def simple_ipv6_traffic (self,
                            stream_id=1,
                            source_traffic_port=1,
                            expected_traffic_tuple={},
                            expected_any_ports=[],
                            expected_any_ports_packets=0,
                            packet_mode="packets",
                            num_packets=5,
                            packets_per_sec=10,
                            num_bursts=1,
                            packets_per_burst=5,
                            frame_len_mode="fixed",
                            frame_len_min=64,
                            frame_len_max=1518,
                            frame_len=300,
                            src_mac='00:01:02:03:04:05',
                            dst_mac='00:06:07:08:09:0a',
                            src_mac_mode="fixed",
                            src_mac_count=1,
                            src_mac_step=1,
                            dst_mac_mode="fixed",
                            dst_mac_count=1,
                            dst_mac_step=1,
                            vlan_enable=False,
                            vlan_id=0,
                            src_ip='1::2',
                            dst_ip='3::4',
                            src_ip_mode="fixed",
                            src_ip_count=1,
                            src_ip_step=1,
                            dst_ip_mode="fixed",
                            dst_ip_count=1,
                            dst_ip_step=1,
                            hlim=64,
                            set_traffic_filter_any_ports="",
                            set_traffic_filter_all_ports="",
                            append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_ipv6(src_mac=src_mac,
                                dst_mac=dst_mac,
                                src_mac_mode=src_mac_mode,
                                src_mac_count=src_mac_count,
                                src_mac_step=src_mac_step,
                                dst_mac_mode=dst_mac_mode,
                                dst_mac_count=dst_mac_count,
                                dst_mac_step=dst_mac_step,
                                src_ip=src_ip,
                                dst_ip=dst_ip,
                                src_ip_mode=src_ip_mode,
                                src_ip_count=src_ip_count,
                                src_ip_step=src_ip_step,
                                dst_ip_mode=dst_ip_mode,
                                dst_ip_count=dst_ip_count,
                                dst_ip_step=dst_ip_step,
                                hlim=hlim,
                                vlan_enable=vlan_enable,
                                vlan_id=vlan_id)
            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)
            return s

    def simple_udpv4_traffic (self,
                              stream_id=1,
                              source_traffic_port=1,
                              expected_traffic_tuple={},
                              expected_any_ports=[],
                              expected_any_ports_packets=0,
                              packet_mode="packets",
                              num_packets=5,
                              packets_per_sec=10,
                              num_bursts=1,
                              packets_per_burst=5,
                              frame_len_mode="fixed",
                              frame_len_min=64,
                              frame_len_max=1518,
                              frame_len=100,
                              src_mac='00:01:02:03:04:05',
                              dst_mac='00:06:07:08:09:0a',
                              src_mac_mode="fixed",
                              src_mac_count=1,
                              src_mac_step=1,
                              dst_mac_mode="fixed",
                              dst_mac_count=1,
                              dst_mac_step=1,
                              vlan_enable=False,
                              vlan_id=0,
                              src_ip='192.168.0.1',
                              dst_ip='192.168.0.2',
                              src_ip_mode="fixed",
                              src_ip_count=1,
                              src_ip_step=1,
                              dst_ip_mode="fixed",
                              dst_ip_count=1,
                              dst_ip_step=1,
                              ttl=64,
                              src_port=1234,
                              dst_port=80,
                              set_traffic_filter_any_ports="",
                              set_traffic_filter_all_ports="",
                              append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_udpv4(src_mac=src_mac,
                                 dst_mac=dst_mac,
                                 src_mac_mode=src_mac_mode,
                                 src_mac_count=src_mac_count,
                                 src_mac_step=src_mac_step,
                                 dst_mac_mode=dst_mac_mode,
                                 dst_mac_count=dst_mac_count,
                                 dst_mac_step=dst_mac_step,
                                 src_ip=src_ip,
                                 dst_ip=dst_ip,
                                 src_ip_mode=src_ip_mode,
                                 src_ip_count=src_ip_count,
                                 src_ip_step=src_ip_step,
                                 dst_ip_mode=dst_ip_mode,
                                 dst_ip_count=dst_ip_count,
                                 dst_ip_step=dst_ip_step,
                                 src_port=src_port,
                                 dst_port=dst_port,
                                 ttl=ttl,
                                 vlan_enable=vlan_enable,
                                 vlan_id=vlan_id)
            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)
            return s

    def simple_udpv6_traffic (self,
                              stream_id=1,
                              source_traffic_port=1,
                              expected_traffic_tuple={},
                              expected_any_ports=[],
                              expected_any_ports_packets=0,
                              packet_mode="packets",
                              num_packets=5,
                              packets_per_sec=10,
                              num_bursts=1,
                              packets_per_burst=5,
                              frame_len_mode="fixed",
                              frame_len_min=64,
                              frame_len_max=1518,
                              frame_len=300,
                              src_mac='00:01:02:03:04:05',
                              dst_mac='00:06:07:08:09:0a',
                              src_mac_mode="fixed",
                              src_mac_count=1,
                              src_mac_step=1,
                              dst_mac_mode="fixed",
                              dst_mac_count=1,
                              dst_mac_step=1,
                              vlan_enable=False,
                              vlan_id=0,
                              src_ip='1::2',
                              dst_ip='3::4',
                              src_ip_mode="fixed",
                              src_ip_count=1,
                              src_ip_step=1,
                              dst_ip_mode="fixed",
                              dst_ip_count=1,
                              dst_ip_step=1,
                              hlim=64,
                              src_port=1234,
                              dst_port=80,
                              set_traffic_filter_any_ports="",
                              set_traffic_filter_all_ports="",
                              append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_udpv6(src_mac=src_mac,
                                 dst_mac=dst_mac,
                                 src_mac_mode=src_mac_mode,
                                 src_mac_count=src_mac_count,
                                 src_mac_step=src_mac_step,
                                 dst_mac_mode=dst_mac_mode,
                                 dst_mac_count=dst_mac_count,
                                 dst_mac_step=dst_mac_step,
                                 src_ip=src_ip,
                                 dst_ip=dst_ip,
                                 src_ip_mode=src_ip_mode,
                                 src_ip_count=src_ip_count,
                                 src_ip_step=src_ip_step,
                                 dst_ip_mode=dst_ip_mode,
                                 dst_ip_count=dst_ip_count,
                                 dst_ip_step=dst_ip_step,
                                 src_port=src_port,
                                 dst_port=dst_port,
                                 hlim=hlim,
                                 vlan_enable=vlan_enable,
                                 vlan_id=vlan_id)
            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)
            return s

    def simple_tcpv4_traffic (self,
                              stream_id=1,
                              source_traffic_port=1,
                              expected_traffic_tuple={},
                              expected_any_ports=[],
                              expected_any_ports_packets=0,
                              packet_mode="packets",
                              num_packets=5,
                              packets_per_sec=10,
                              num_bursts=1,
                              packets_per_burst=5,
                              frame_len_mode="fixed",
                              frame_len_min=64,
                              frame_len_max=1518,
                              frame_len=100,
                              src_mac='00:01:02:03:04:05',
                              dst_mac='00:06:07:08:09:0a',
                              src_mac_mode="fixed",
                              src_mac_count=1,
                              src_mac_step=1,
                              dst_mac_mode="fixed",
                              dst_mac_count=1,
                              dst_mac_step=1,
                              vlan_enable=False,
                              vlan_id=0,
                              src_ip='192.168.0.1',
                              dst_ip='192.168.0.2',
                              src_ip_mode="fixed",
                              src_ip_count=1,
                              src_ip_step=1,
                              dst_ip_mode="fixed",
                              dst_ip_count=1,
                              dst_ip_step=1,
                              ttl=64,
                              src_port=1234,
                              dst_port=80,
                              set_traffic_filter_any_ports="",
                              set_traffic_filter_all_ports="",
                              append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_tcpv4(src_mac=src_mac,
                                 dst_mac=dst_mac,
                                 src_mac_mode=src_mac_mode,
                                 src_mac_count=src_mac_count,
                                 src_mac_step=src_mac_step,
                                 dst_mac_mode=dst_mac_mode,
                                 dst_mac_count=dst_mac_count,
                                 dst_mac_step=dst_mac_step,
                                 src_ip=src_ip,
                                 dst_ip=dst_ip,
                                 src_ip_mode=src_ip_mode,
                                 src_ip_count=src_ip_count,
                                 src_ip_step=src_ip_step,
                                 dst_ip_mode=dst_ip_mode,
                                 dst_ip_count=dst_ip_count,
                                 dst_ip_step=dst_ip_step,
                                 src_port=src_port,
                                 dst_port=dst_port,
                                 ttl=ttl,
                                 vlan_enable=vlan_enable,
                                 vlan_id=vlan_id)
            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)
            return s

    def simple_tcpv6_traffic (self,
                              stream_id=1,
                              source_traffic_port=1,
                              expected_traffic_tuple={},
                              expected_any_ports=[],
                              expected_any_ports_packets=0,
                              packet_mode="packets",
                              num_packets=5,
                              packets_per_sec=10,
                              num_bursts=1,
                              packets_per_burst=5,
                              frame_len_mode="fixed",
                              frame_len_min=64,
                              frame_len_max=1518,
                              frame_len=300,
                              src_mac='00:01:02:03:04:05',
                              dst_mac='00:06:07:08:09:0a',
                              src_mac_mode="fixed",
                              src_mac_count=1,
                              src_mac_step=1,
                              dst_mac_mode="fixed",
                              dst_mac_count=1,
                              dst_mac_step=1,
                              vlan_enable=False,
                              vlan_id=0,
                              src_ip='1::2',
                              dst_ip='3::4',
                              src_ip_mode="fixed",
                              src_ip_count=1,
                              src_ip_step=1,
                              dst_ip_mode="fixed",
                              dst_ip_count=1,
                              dst_ip_step=1,
                              hlim=64,
                              src_port=1234,
                              dst_port=80,
                              set_traffic_filter_any_ports="",
                              set_traffic_filter_all_ports="",
                              append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_tcpv6(src_mac=src_mac,
                                 dst_mac=dst_mac,
                                 src_mac_mode=src_mac_mode,
                                 src_mac_count=src_mac_count,
                                 src_mac_step=src_mac_step,
                                 dst_mac_mode=dst_mac_mode,
                                 dst_mac_count=dst_mac_count,
                                 dst_mac_step=dst_mac_step,
                                 src_ip=src_ip,
                                 dst_ip=dst_ip,
                                 src_ip_mode=src_ip_mode,
                                 src_ip_count=src_ip_count,
                                 src_ip_step=src_ip_step,
                                 dst_ip_mode=dst_ip_mode,
                                 dst_ip_count=dst_ip_count,
                                 dst_ip_step=dst_ip_step,
                                 src_port=src_port,
                                 dst_port=dst_port,
                                 hlim=hlim,
                                 vlan_enable=vlan_enable,
                                 vlan_id=vlan_id)
            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)
            return s

    def simple_vxlan_traffic (self,
                              stream_id=1,
                              source_traffic_port=1,
                              expected_traffic_tuple={},
                              expected_any_ports=[],
                              expected_any_ports_packets=0,
                              vxlan_src_mac='00:01:02:03:04:05',
                              vxlan_dst_mac='00:06:07:08:09:0a',
                              vxlan_src_ip='192.168.0.1',
                              vxlan_dst_ip='192.168.0.2',
                              vxlan_udp_port=11638,
                              vxlan_vni=0x1234,
                              vxlan_udp_chksum=False,
                              packet_mode="packets",
                              num_packets=5,
                              packets_per_sec=10,
                              num_bursts=1,
                              packets_per_burst=5,
                              frame_len_mode="fixed",
                              frame_len_min=64,
                              frame_len_max=1518,
                              frame_len=300,
                              src_mac='00:01:02:03:04:05',
                              dst_mac='00:06:07:08:09:0a',
                              src_mac_mode="fixed",
                              src_mac_count=1,
                              src_mac_step=1,
                              dst_mac_mode="fixed",
                              dst_mac_count=1,
                              dst_mac_step=1,
                              vlan_enable=False,
                              vlan_id=0,
                              src_ip='1.1.1.1',
                              dst_ip='2.2.2.2',
                              src_ip_mode="fixed",
                              src_ip_count=1,
                              src_ip_step=1,
                              dst_ip_mode="fixed",
                              dst_ip_count=1,
                              dst_ip_step=1,
                              hlim=64,
                              ttl=64,
                              src_port=1234,
                              dst_port=80,
                              inner_protocol="tcpv4",
                              set_traffic_filter_any_ports="",
                              set_traffic_filter_all_ports="",
                              append_traffic_filter_all_ports=""):

            s = self.add_stream(port_name=source_traffic_port,
                                stream_id=stream_id,
                                rx_all_ports=expected_traffic_tuple,
                                rx_any_ports=expected_any_ports,
                                rx_any_ports_packets=expected_any_ports_packets)

            s.add_template_vxlan_header(src_mac=vxlan_src_mac,
                                        dst_mac=vxlan_dst_mac,
                                        src_ip=vxlan_src_ip,
                                        dst_ip=vxlan_dst_ip,
                                        src_port=vxlan_udp_port,
                                        vxlan_vni=vxlan_vni,
                                        udp_chksum=vxlan_udp_chksum)

            if inner_protocol == "tcpv4":
                s.add_template_tcpv4(src_mac=src_mac,
                                     dst_mac=dst_mac,
                                     src_mac_mode=src_mac_mode,
                                     src_mac_count=src_mac_count,
                                     src_mac_step=src_mac_step,
                                     dst_mac_mode=dst_mac_mode,
                                     dst_mac_count=dst_mac_count,
                                     dst_mac_step=dst_mac_step,
                                     src_ip=src_ip,
                                     dst_ip=dst_ip,
                                     src_ip_mode=src_ip_mode,
                                     src_ip_count=src_ip_count,
                                     src_ip_step=src_ip_step,
                                     dst_ip_mode=dst_ip_mode,
                                     dst_ip_count=dst_ip_count,
                                     dst_ip_step=dst_ip_step,
                                     src_port=src_port,
                                     dst_port=dst_port,
                                     ttl=ttl,
                                     vlan_enable=vlan_enable,
                                     vlan_id=vlan_id)
            elif inner_protocol == "tcpv6":
                s.add_template_tcpv6(src_mac=src_mac,
                                     dst_mac=dst_mac,
                                     src_mac_mode=src_mac_mode,
                                     src_mac_count=src_mac_count,
                                     src_mac_step=src_mac_step,
                                     dst_mac_mode=dst_mac_mode,
                                     dst_mac_count=dst_mac_count,
                                     dst_mac_step=dst_mac_step,
                                     src_ip=src_ip,
                                     dst_ip=dst_ip,
                                     src_ip_mode=src_ip_mode,
                                     src_ip_count=src_ip_count,
                                     src_ip_step=src_ip_step,
                                     dst_ip_mode=dst_ip_mode,
                                     dst_ip_count=dst_ip_count,
                                     dst_ip_step=dst_ip_step,
                                     src_port=src_port,
                                     dst_port=dst_port,
                                     hlim=hlim,
                                     vlan_enable=vlan_enable,
                                     vlan_id=vlan_id)
            elif inner_protocol == "udpv4":
                s.add_template_tcpv6(src_mac=src_mac,
                                     dst_mac=dst_mac,
                                     src_mac_mode=src_mac_mode,
                                     src_mac_count=src_mac_count,
                                     src_mac_step=src_mac_step,
                                     dst_mac_mode=dst_mac_mode,
                                     dst_mac_count=dst_mac_count,
                                     dst_mac_step=dst_mac_step,
                                     src_ip=src_ip,
                                     dst_ip=dst_ip,
                                     src_ip_mode=src_ip_mode,
                                     src_ip_count=src_ip_count,
                                     src_ip_step=src_ip_step,
                                     dst_ip_mode=dst_ip_mode,
                                     dst_ip_count=dst_ip_count,
                                     dst_ip_step=dst_ip_step,
                                     src_port=src_port,
                                     dst_port=dst_port,
                                     ttl=ttl,
                                     vlan_enable=vlan_enable,
                                     vlan_id=vlan_id)
            elif inner_protocol == "udpv6":
                s.add_template_tcpv6(src_mac=src_mac,
                                     dst_mac=dst_mac,
                                     src_mac_mode=src_mac_mode,
                                     src_mac_count=src_mac_count,
                                     src_mac_step=src_mac_step,
                                     dst_mac_mode=dst_mac_mode,
                                     dst_mac_count=dst_mac_count,
                                     dst_mac_step=dst_mac_step,
                                     src_ip=src_ip,
                                     dst_ip=dst_ip,
                                     src_ip_mode=src_ip_mode,
                                     src_ip_count=src_ip_count,
                                     src_ip_step=src_ip_step,
                                     dst_ip_mode=dst_ip_mode,
                                     dst_ip_count=dst_ip_count,
                                     dst_ip_step=dst_ip_step,
                                     src_port=src_port,
                                     dst_port=dst_port,
                                     hlim=hlim,
                                     vlan_enable=vlan_enable,
                                     vlan_id=vlan_id)
            elif inner_protocol == "ipv4":
                s.add_template_ipv4(src_mac=src_mac,
                                     dst_mac=dst_mac,
                                     src_mac_mode=src_mac_mode,
                                     src_mac_count=src_mac_count,
                                     src_mac_step=src_mac_step,
                                     dst_mac_mode=dst_mac_mode,
                                     dst_mac_count=dst_mac_count,
                                     dst_mac_step=dst_mac_step,
                                     src_ip=src_ip,
                                     dst_ip=dst_ip,
                                     src_ip_mode=src_ip_mode,
                                     src_ip_count=src_ip_count,
                                     src_ip_step=src_ip_step,
                                     dst_ip_mode=dst_ip_mode,
                                     dst_ip_count=dst_ip_count,
                                     dst_ip_step=dst_ip_step,
                                     ttl=ttl,
                                     vlan_enable=vlan_enable,
                                     vlan_id=vlan_id)
            elif inner_protocol == "ipv6":
                s.add_template_ipv6(src_mac=src_mac,
                                     dst_mac=dst_mac,
                                     src_mac_mode=src_mac_mode,
                                     src_mac_count=src_mac_count,
                                     src_mac_step=src_mac_step,
                                     dst_mac_mode=dst_mac_mode,
                                     dst_mac_count=dst_mac_count,
                                     dst_mac_step=dst_mac_step,
                                     src_ip=src_ip,
                                     dst_ip=dst_ip,
                                     src_ip_mode=src_ip_mode,
                                     src_ip_count=src_ip_count,
                                     src_ip_step=src_ip_step,
                                     dst_ip_mode=dst_ip_mode,
                                     dst_ip_count=dst_ip_count,
                                     dst_ip_step=dst_ip_step,
                                     hlim=hlim,
                                     vlan_enable=vlan_enable,
                                     vlan_id=vlan_id)
            else:
                pass

            s.set_packet_control(packet_mode=packet_mode,
                                 num_packets=num_packets,
                                 packets_per_sec = packets_per_sec,
                                 num_bursts=num_bursts,
                                 packets_per_burst=packets_per_burst)
            s.set_frame_len_mode(frame_len_mode=frame_len_mode,
                                 frame_len_min=frame_len_min,
                                 frame_len_max=frame_len_max,
                                 frame_len=frame_len)
            if set_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.set_display_filter_all_ports(rx_port, set_traffic_filter_all_ports)
            if append_traffic_filter_all_ports != "":
                for rx_port in expected_traffic_tuple:
                    s.append_display_filter_all_ports(rx_port, append_traffic_filter_all_ports)
            if set_traffic_filter_any_ports != "":
                s.set_display_filter_any_ports(set_traffic_filter_any_ports)

class TrafficPort:
    """Class used hold the transmit port information.

    Each TrafficPort instance holds streams_list that conatins TrafficStream object, which
    has all the properties such as packet size, packet rate, number, burst mode,
    protocotols, protocol values (src / dst mac, ip, vlan, etc...)

    The actual traffic stream is not applied / pushed to Drone server until apply_streams
    method is called.
    """

    def __init__(self, port_id, port_name):
        self.port_name = port_name
        self.port_id = port_id
        self.stream_list = dict()

    def set_sequential(self, server):
        self.port_mode = "sequential"
        ost_port_config = ost_pb.PortConfigList()
        ost_port = ost_port_config.port.add()
        ost_port.port_id.id = self.port_id
        ost_port.transmit_mode = ost_pb.kSequentialTransmit
        server.modifyPort(ost_port_config)

    def set_interleaved(self, server):
        self.port_mode = "interleaved"
        ost_port_config = ost_pb.PortConfigList()
        ost_port = ost_port_config.port.add()
        ost_port.port_id.id = self.port_id
        ost_port.transmit_mode = ost_pb.kInterleavedTransmit
        server.modifyPort(ost_port_config)

    def add_stream(self, server, stream_id, rx_all_ports={}, rx_any_ports=[], rx_any_ports_packets=0):
        if stream_id not in self.stream_list:
            self.stream_list[stream_id] = traffic_streams.TrafficStream(port_id = self.port_id,
                                                                        stream_id = stream_id,
                                                                        rx_all_ports = rx_all_ports,
                                                                        rx_any_ports = rx_any_ports,
                                                                        rx_any_ports_packets = rx_any_ports_packets)
            server.addStream(self.stream_list[stream_id].ost_stream)
            return self.stream_list[stream_id]

    def del_stream(self, server, stream_id):
        if stream_id in self.stream_list:
            server.deleteStream(self.stream_list[stream_id].ost_stream)
            port_cfg = ost_pb.PortId()
            port_cfg.id = self.port_id
            stream_cfg = ost_pb.StreamConfigList()
            stream_cfg.port_id.CopyFrom(port_cfg)
            server.modifyStream(stream_cfg)
            del self.stream_list[stream_id]

    def del_all_streams(self, server):
        for stream_id in self.stream_list:
            server.deleteStream(self.stream_list[stream_id].ost_stream)
            port_cfg = ost_pb.PortId()
            port_cfg.id = self.port_id
            stream_cfg = ost_pb.StreamConfigList()
            stream_cfg.port_id.CopyFrom(port_cfg)
            server.modifyStream(stream_cfg)
        self.stream_list = dict()

    def apply_streams(self, server):
        for stream_id in self.stream_list:
            stream = self.stream_list[stream_id]
            stream.apply_stream(server)
