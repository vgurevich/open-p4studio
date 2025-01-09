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


from ixnetwork_restpy.testplatform.testplatform import TestPlatform
from ixnetwork_restpy.assistants.statistics.statviewassistant import StatViewAssistant
import yaml
try:
    import ptf
except:
    pass

class TrafficGen(object):
    '''
    TrafficGen is the main object used to control and manage traffic generator.

    Attributes:
        api_server_ip = The ip address for device that is listening for rest service
        username = user name to login with
        password = password to login with
        tg_type = traffic generator type: "ixia", "spirent", "elastic_stream", etc...
        stream = dictionary of Stream objects that can be managed in this traffic generator device
    '''
    def __init__(self, api_server_ip=None, username=None, password=None, tg_type=None):
        try:
            tg_topo = ptf.testutils.test_param_get('tg_topo')
            tg_json = yaml.safe_load(open(tg_topo))
            api_server_ip = tg_json['tg_chassis_list'][0]['chassis']
            tg_type = tg_json['tg_chassis_list'][0]['type']
            username = tg_json['tg_chassis_list'][0]['username']
            password = tg_json['tg_chassis_list'][0]['password']
        except:
            if api_server_ip == None:
                print ("Either ptf test_param tg_topo not set, or api_server_ip not given.  Exiting...")
                exit(1)

        self.api_server_ip = api_server_ip
        self.username = username
        self.password = password
        self.tg_type = tg_type
        self.stream = {}

    def connect(self, port_list=None, new_config=True):
        '''
            connect method to the traffic generator interface.
            - It will create a new session if it does not exist for this user
            and reuse the same session that user previously created (to save time)
            - For Ixia it will always create a new IxNetwork config after session is configured
        '''
        try:
            tg_topo = ptf.testutils.test_param_get('tg_topo')
            tg_json = yaml.safe_load(open(tg_topo))
            tg_port_dict = {}
            for tg_port in tg_json['tg_port_list']:
                vport = tg_port['name']
                chassis = tg_port['chassis']
                card = tg_port['card']
                port = tg_port['port']
                tg_port_dict[vport] = [chassis, card, port]
            port_list = tg_port_dict
        except:
            if port_list == None:
                print ("port_list or ptf test_param is required for TrafficGen connect method, exiting...")
                exit(1)

        # Login to the test server and get the api key
        test_platform = TestPlatform(self.api_server_ip, platform='linux')
        test_platform.Authenticate(self.username, self.password)
        self.api_key = test_platform.ApiKey

        session_list = test_platform.Sessions.find()
        if len(session_list) == 0:
            # Create a new session if there is no api server session for this user
            self.session = test_platform.Sessions.add()
        else:
            # Reuse the first api server session for this user
            self.session = session_list[0]

        self.session_id = self.session.Id
        self.ixnetwork = self.session.Ixnetwork

        # Create a new IxNetwork config if needed
        if new_config:
            self.ixnetwork.NewConfig()

        # Enable Ixia traffic loss duration counter by default
        self.ixnetwork.Traffic.Statistics.PacketLossDuration.Enabled = True

        # Create Ixia api port list and map to the virtual port name
        test_ports = []
        test_vports = []
        self.vports_obj = {}

        for vport in port_list.keys():
            port = port_list[vport]
            test_ports.append(dict (Arg1=port[0], Arg2=port[1], Arg3=port[2]))

            vport_obj = self.ixnetwork.Vport.add(Name=vport)
            test_vports.append(vport_obj.href)
            self.vports_obj[vport] = vport_obj

        self.ixnetwork.AssignPorts(test_ports, [], test_vports, True)

    def disconnect(self, destroy_api_server=False):
        '''
          disconnect method to disconnect form traffic generator.
          - For Ixia and Spirent, will leave the api server / session running
          till next run where new config is created.
        '''
        if destroy_api_server:
            self.session.remove()

    def create_stream(self,
                      src_vport,
                      dst_vport,
                      stream_name,
                      frame_size_type="fixed",
                      frame_size=128,
                      frame_rate_type="percentLineRate",
                      frame_rate=10,
                      transmit_type="continuous",
                      duration=120,
                      frame_count=100000,
                      ):
        '''
           method to instantiate a Strem object and added to the TrafficGen.stream diciontary.
           Currently only support one to one port mapping, and not protocol / multiport

           src_vport: <string> source port name to generate traffic
           dst_vport: <string> destination port name to receive traffic
           stream_name: <string> unique stream_name to identify this traffic
           frame_size_type: <string> "fixed" (default), "increment", "random"
           frame_size: <int> default 128 bytes
           frame_rate_type: <string> bitsPerSecond|framesPerSecond|interPacketGap|percentLineRate (default)
           frame_rate: <int> 10 (default)  frame rate based on frame_rate_type unit.  Default is 10% line rate
           transmit_type: <string> auto|continuous(default)|custom|fixedDuration|fixedFrameCount|fixedIterationCount
           duration: <int> 120 (default)  transmit duration.  Ignored if transmit_type set to value other than fixedDuration
           frame_count: <int> 100000 (default) transmit frame count.  Ignored if transmit_type set to value other than fixedFrameCount
        '''
        stream_obj = Stream(self.ixnetwork, self.vports_obj[src_vport], self.vports_obj[dst_vport], stream_name)

        stream_obj.frame_rate(frame_rate_type=frame_rate_type, frame_rate=frame_rate)
        stream_obj.frame_size(frame_size_type=frame_size_type, frame_size=frame_size)
        stream_obj.transmission_control(transmit_type=transmit_type, duration=duration, frame_count=frame_count)

        self.stream[stream_name] = stream_obj

        return stream_obj

    def start_transmit(self):
        '''
           method to start transmit all streams on traffic generator in current session
        '''
        # Generate traffic items, this is needed to resolve arp, or get info from protocol server
        #for stream in self.ixnetwork.Traffic.TrafficItem.find():
        #    stream.Generate()
        for s in self.stream.keys():
            self.stream[s].traffic_item.Generate()
        self.ixnetwork.Traffic.Apply()
        self.stats = Stats(self.ixnetwork)
        self.ixnetwork.Traffic.Start()

    def stop_transmit(self):
        '''
           method to stop transmit all streams on traffic generator in current session
        '''
        self.ixnetwork.Traffic.Stop()

    def remove(self,name):
        self.stream[name].traffic_item.remove()

    def shut_port(self, vport_name):
        '''
           method to shut down port based on portname
        '''
        self.vports_obj[vport_name].LinkUpDn("down")

    def noshut_port(self, vport_name):
        '''
           method to no shut port based on portname
        '''
        self.vports_obj[vport_name].LinkUpDn("up")

    def set_port_fec(self, vport_name, fec_mode="off"):
        '''
           method to set fec mode based on portname
           vport: <string> port name
           fec_mode: <string> off(default) | rs
        '''
        port_type = self.ixnetwork.Vport.find(Name=vport_name).Type
        port_type = port_type[0].upper() + port_type[1:]

        if "novus" in port_type.lower():
            exec("self.vports_obj[vport_name].L1Config.%s.IeeeL1Defaults = False" % port_type)

        if fec_mode == "off":
            exec("self.vports_obj[vport_name].L1Config.%s.EnableRsFec = False" % port_type)
        else:
            if "novus" in port_type.lower():
                exec("self.vports_obj[vport_name].L1Config.%s.FirecodeForceOn = False" % port_type)
                exec("self.vports_obj[vport_name].L1Config.%s.RsFecForceOn = True" % port_type)
                exec("self.vports_obj[vport_name].L1Config.%s.RsFecAdvertise = True" % port_type)
                exec("self.vports_obj[vport_name].L1Config.%s.EnableRsFec = True" % port_type)
            else:
                exec("self.vports_obj[vport_name].L1Config.%s.EnableRsFec = True" % port_type)

    def set_port_an(self, vport_name, an="off"):
        '''
           method to set autoneg mode based on portname
           vport: <string> port name
           an: <string> off (default) | on
        '''
        port_type = self.ixnetwork.Vport.find(Name=vport_name).Type
        port_type = port_type[0].upper() + port_type[1:]

        if "novus" in port_type.lower():
            exec("self.vports_obj[vport_name].L1Config.%s.IeeeL1Defaults = False" % port_type)
        if an == "off":
            exec("self.vports_obj[vport_name].L1Config.%s.EnableAutoNegotiation = False" % port_type)
        else:
            exec("self.vports_obj[vport_name].L1Config.%s.EnableAutoNegotiation = True" % port_type)

class Stream(object):
    '''
        Strem object to management individual streams in the traffic generator
        Currently only support port to port stream, and not protocol / multiport streams
        Attributes:
        self.ixnetwork = ixnetwork object to be used for main configuration
        self.src_vport = source port name for the traffic stream
        self.dst_vport = destination port name for the traffic stream
        self.stream_name = stream name
        self.protocol = Mapping between protocol name and Ixia protocol template name
    '''
    def __init__(self, ixnetwork, src_vport, dst_vport, stream_name):
        self.ixnetwork = ixnetwork
        self.src_vport = src_vport
        self.dst_vport = dst_vport
        self.stream_name = stream_name
        self.protocol = {}

        # Creating traffic item
        self.traffic_item = self.ixnetwork.Traffic.TrafficItem.add(Name=self.stream_name,
                                                BiDirectional=False,
                                                TrafficType='raw',
                                                TrafficItemType='l2L3'
                                             )
        # Creating end points for this traffic item
        self.traffic_item.EndpointSet.add(Sources=self.src_vport.Protocols.find(),
                                          Destinations=self.dst_vport.Protocols.find())

        # Enable stream tracking with each traffic item by default
        self.traffic_item.Tracking.find()[0].TrackBy = ['flowGroup0']

        # Storing the stream_cfg handle for stream control and stream modification
        self.stream_cfg = self.traffic_item.ConfigElement.find()[0]

        # Mapping between protocol name and Ixia protocol template name
        self.protocol['ethernet'] = 'Ethernet II'
        self.protocol['vlan'] = 'VLAN$'
        self.protocol['ipv4'] = 'IPv4'
        self.protocol['ipv6'] = 'IPv6'
        self.protocol['tcp'] = 'TCP'
        self.protocol['udp'] = 'UDP'
        self.protocol['pfcpause'] = 'PFC PAUSE'

    def frame_rate(self,
                   frame_rate_type="percentLineRate",
                   frame_rate=10):
        '''
        Stream level method for frame rate
        |  Rate
        |      The rate at which packet is transmitted.
        |

        |  Type
        |      Sets the frame rate types.
        |              str(bitsPerSecond|framesPerSecond|interPacketGap|percentLineRate)
        |
        '''
        self.stream_cfg.FrameRate.Type = frame_rate_type
        self.stream_cfg.FrameRate.Rate = frame_rate

    def frame_size(self,
                   frame_size_type="fixed",
                   frame_size=128,
                   increment_from=64,
                   increment_step=1,
                   increment_to=9216,
                   preset_distribution="imix",
                   quad_gaussian=[],
                   random_min=64,
                   random_max=9216,
                   weighted_pairs=[],
                   weighted_range_pairs=[]
                   ):
        '''
        Stream level method for frame size
        |  FixedSize
        |      Sets all frames to a constant specified size.
        |              number
        |
        |  IncrementFrom
        |      Specifies the Start Value if the Frame Size is incremented.
        |              number
        |
        |  IncrementStep
        |      Specifies the Step Value if the Frame Size is Increment.
        |              number
        |
        |  IncrementTo
        |      Specifies the Final Value if the Frame Size is Increment.
        |              number
        |
        |  PresetDistribution
        |      If set, Frame Size is set to IMIX.
        |              str(cisco|imix|ipSecImix|ipV6Imix|rprQuar|rprTri|standardImix|tcpImix|tolly)
        |
        |  QuadGaussian
        |      This option allows to set frames to use a calculated distribution of Frame sizes. Quad Gaussian is the superposition of four Gaussian distributions. The user can specify the center (or mean), width of half maximum, and weight of each Gaussian distribution. The distribution is then normalized to a single distribution and generates the random numbers according to the normalized distribution.
        |              list(number)
        |
        |  RandomMax
        |              number
        |
        |
        |  RandomMin
        |              number
        |
        |  Type
        |              str(auto|fixed|increment|presetDistribution|quadGaussian|random|weightedPairs)
        |
        |  WeightedPairs
        |              list(number)
        |
        |  WeightedRangePairs
        |              list(dict(arg1:number,arg2:number,arg3:number))
        |
        '''
        self.stream_cfg.FrameSize.Type = frame_size_type
        self.stream_cfg.FrameSize.FixedSize = frame_size
        self.stream_cfg.FrameSize.IncrementFrom = increment_from
        self.stream_cfg.FrameSize.IncrementStep = increment_step
        self.stream_cfg.FrameSize.IncrementTo = increment_to
        self.stream_cfg.FrameSize.PresetDistribution= preset_distribution
        self.stream_cfg.FrameSize.QuadGaussian= quad_gaussian
        self.stream_cfg.FrameSize.RandomMax = random_max
        self.stream_cfg.FrameSize.RandomMin = random_min
        self.stream_cfg.FrameSize.WeightedPairs = weighted_pairs
        self.stream_cfg.FrameSize.WeightedRangePairs = weighted_range_pairs

    def transmission_control(self,
                             transmit_type="continuous",
                             duration="120",
                             frame_count="100000"):
        '''
        Stream level method for transmission control
        |  duration
        |      Indicates the time duration.
        |

        |  transmit_type
        |      The Transmission Control types.
        |              str(auto|continuous|custom|fixedDuration|fixedFrameCount|fixedIterationCount)

        |  frame_count
        |      Specifies Fixed Packet Count when Transmission Mode is Interleaved.
        |              number
        '''
        self.stream_cfg.TransmissionControl.Type = transmit_type
        self.stream_cfg.TransmissionControl.Duration = duration
        self.stream_cfg.TransmissionControl.FrameCount = frame_count

    def enable(self):
        '''
        Stream level method to enable stream
        '''
        self.traffic_item.Enabled = True

    def disable(self):
        '''
        Stream level method to disable stream
        '''
        self.traffic_item.Enabled = False

    def pause(self):
        '''
        Stream level method to pause stream
        '''
        self.traffic_item.Suspend = True

    def resume(self):
        '''
        Stream level method to resume stream
        '''
        self.traffic_item.Suspend = False

    def _createPacketHeader(self, trafficItemObj, packetHeaderToAdd=None, appendToStack=None):
        '''
        internal stream method to create package header based on Ixia protocol template
        '''
        configElement = trafficItemObj.ConfigElement.find()[0]

        # Do the followings to add packet headers on the new traffic item

        # Uncomment this to show a list of all the available protocol templates to create (packet headers)
        #for protocolHeader in ixNetwork.Traffic.ProtocolTemplate.find():
        #    ixNetwork.info('Protocol header: {}'.format(protocolHeader))

        # 1> Get the <new packet header> protocol template from the ProtocolTemplate list.
        packetHeaderProtocolTemplate = self.ixnetwork.Traffic.ProtocolTemplate.find(DisplayName=packetHeaderToAdd)

        # 2> Append the <new packet header> object after the specified packet header stack.
        appendToStackObj = configElement.Stack.find(DisplayName=appendToStack)
        appendToStackObj.Append(Arg2=packetHeaderProtocolTemplate)

        #Remove Ethernet Header and keep only PFC PAUSE"
        if packetHeaderToAdd == "PFC PAUSE":
            removeStackObj = configElement.Stack.find(DisplayName=self.protocol['ethernet'])
            removeStackObj.Remove()

        # 3> Get the new packet header stack to use it for appending an IPv4 stack after it.
        # Look for the packet header object and stack ID.
        packetHeaderStackObj = configElement.Stack.find(DisplayName=packetHeaderToAdd)

        # 4> In order to modify the fields, get the field object
        packetHeaderFieldObj = packetHeaderStackObj.Field.find()

        return packetHeaderFieldObj

    def _add_ethernet(self, append_to_stack=None,
                     smac = "00:00:01:33:55:77",
                     smac_step = "00:00:00:00:00:01",
                     smac_count = 1,
                     smac_value_type = "singleValue",
                     dmac = "00:00:02:44:66:88",
                     dmac_step = "00:00:00:00:00:01",
                     dmac_count = 1,
                     dmac_value_type = "singleValue"):
        '''
        internal method to add ethernet header (can append to custom stack)
        '''
        # Ethernet header doesn't need to be created.  It is there by default. Just do a find for the Ethernet stack object.
        if append_to_stack == None:
            ethernetStackObj = self.traffic_item.ConfigElement.find()[0].Stack.find(DisplayName=self.protocol['ethernet'])
        else:
            ethernetStackObj = self._createPacketHeader(self.traffic_item, packetHeaderToAdd=self.protocol['ethernet'], appendToStack=self.protocol[append_to_stack])

        ethernetDstField = ethernetStackObj.Field.find(DisplayName='Destination MAC Address')
        ethernetDstField.ValueType = dmac_value_type
        ethernetDstField.SingleValue = dmac
        ethernetDstField.StartValue = dmac
        ethernetDstField.StepValue = dmac_step
        ethernetDstField.CountValue = dmac_count

        ethernetSrcField = ethernetStackObj.Field.find(DisplayName='Source MAC Address')
        ethernetSrcField.ValueType = smac_value_type
        ethernetSrcField.SingleValue = smac
        ethernetSrcField.StartValue = smac
        ethernetSrcField.StepValue = smac_step
        ethernetSrcField.CountValue = smac_count

    def add_ethernet(self,
                     smac = "00:00:01:33:55:77",
                     smac_step = "00:00:00:00:00:01",
                     smac_count = 1,
                     smac_value_type = "singleValue",
                     dmac = "00:00:02:44:66:88",
                     dmac_step = "00:00:00:00:00:01",
                     dmac_count = 1,
                     dmac_value_type = "singleValue"):
        '''
            stream method to add ethernet header
                smac: <string> first source mac in xx:xx:xx:xx:xx:xx format
                smac_step: <string> source mac step value in xx:xx:xx:xx:xx:xx format
                smac_count = <int> source mac count (ignored if value type is singleValue)
                smac_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dmac: <string> first destination mac in xx:xx:xx:xx:xx:xx format
                dmac_step: <string> destination mac step value in xx:xx:xx:xx:xx:xx format
                dmac_count = <int> destination mac count (ignored if value type is singleValue)
                dmac_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''
        self._add_ethernet(append_to_stack=None,
                     smac = smac,
                     smac_step = smac_step,
                     smac_count = smac_count,
                     smac_value_type = smac_value_type,
                     dmac = dmac,
                     dmac_step = dmac_step,
                     dmac_count = dmac_count,
                     dmac_value_type = dmac_value_type)

    def _add_vlan(self, append_to_stack='ethernet',
                 vlan_id = 1000,
                 vlan_value_type = "singleValue",
                 vlan_step = 1,
                 vlan_count = 1,
                 vlan_priority = 0,
                 vlan_priority_value_type = "singleValue",
                 vlan_priority_step = 1,
                 vlan_priority_count = 1):
        '''
        internal method to add vlan header (can append to custom stack)
        '''
        vlanFieldObj = self._createPacketHeader(self.traffic_item, packetHeaderToAdd=self.protocol['vlan'], appendToStack=self.protocol[append_to_stack])
        vlanFieldObj.find(DisplayName='VLAN Priority').Auto = False
        vlanFieldObj.find(DisplayName='VLAN Priority').SingleValue = vlan_priority
        vlanFieldObj.find(DisplayName='VLAN-ID').Auto = False
        vlanFieldObj.find(DisplayName='VLAN-ID').SingleValue = vlan_id

    def add_vlan(self,
                 vlan_id = 1000,
                 vlan_value_type = "singleValue",
                 vlan_step = 1,
                 vlan_count = 1,
                 vlan_priority = 0,
                 vlan_priority_value_type = "singleValue",
                 vlan_priority_step = 1,
                 vlan_priority_count = 1):
        '''
            stream method to add vlan header
                vlan_id: <int> first vlan id
                vlan_step: <int> vlan step value
                vlan_count = <int> vlan  count (ignored if value type is singleValue)
                vlan_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                vlan_priority: <int> first vlan_priority
                vlan_priority_step: <int> vlan_priority step value
                vlan_priority_count = <int> vlan_priority  count (ignored if value type is singleValue)
                vlan_priority_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''
        self._add_vlan(append_to_stack='ethernet',
                 vlan_id = vlan_id,
                 vlan_value_type = vlan_value_type,
                 vlan_step = vlan_step,
                 vlan_count = vlan_count,
                 vlan_priority = vlan_priority,
                 vlan_priority_value_type = vlan_priority_value_type,
                 vlan_priority_step = vlan_priority_step,
                 vlan_priority_count = vlan_priority_count)

    def _add_ipv4(self, append_to_stack='ethernet',
                 sip= "1.1.1.1",
                 sip_value_type = "singleValue",
                 sip_step = "0.0.0.1",
                 sip_count = 1,
                 dip = "2.2.2.2",
                 dip_value_type = "singleValue",
                 dip_step = "0.0.0.1",
                 dip_count = 1):
        '''
        internal method to add ipv4 header (can append to custom stack)
        '''
        outerIpv4FieldObj = self._createPacketHeader(self.traffic_item, packetHeaderToAdd=self.protocol['ipv4'], appendToStack=self.protocol[append_to_stack])

        outerIpv4FieldObj.find(DisplayName='Source Address').ValueType = sip_value_type
        outerIpv4FieldObj.find(DisplayName='Source Address').SingleValue = sip
        outerIpv4FieldObj.find(DisplayName='Source Address').StartValue = sip
        outerIpv4FieldObj.find(DisplayName='Source Address').StepValue = sip_step
        outerIpv4FieldObj.find(DisplayName='Source Address').CountValue = sip_count

        outerIpv4FieldObj.find(DisplayName='Destination Address').ValueType = dip_value_type
        outerIpv4FieldObj.find(DisplayName='Destination Address').SingleValue = dip
        outerIpv4FieldObj.find(DisplayName='Destination Address').StartValue = dip
        outerIpv4FieldObj.find(DisplayName='Destination Address').StepValue = dip_step
        outerIpv4FieldObj.find(DisplayName='Destination Address').CountValue = dip_count

    def add_ipv4(self,
                 after_vlan = False,
                 sip = "1.1.1.1",
                 sip_value_type = "singleValue",
                 sip_step = "0.0.0.1",
                 sip_count = 1,
                 dip = "2.2.2.2",
                 dip_value_type = "singleValue",
                 dip_step = "0.0.0.1",
                 dip_count = 1):
        '''
            stream method to add ipv4 header
                after_vlan: <bool> True if append after vlan tag, False if append after Ethrenet header
                sip: <string> first source ip in x.x.x.x format
                sip_step: <string> source ip step value in x.x.x.x format
                sip_count = <int> source ip count (ignored if value type is singleValue)
                sip_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dip: <string> first destination ip in x.x.x.x format
                dip_step: <string> destination ip step value in x.x.x.x format
                dip_count = <int> destination ip count (ignored if value type is singleValue)
                dip_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''
        if not after_vlan:
            stack = 'ethernet'
        else:
            stack = 'vlan'

        self._add_ipv4(append_to_stack=stack,
                 sip = sip,
                 sip_value_type = sip_value_type,
                 sip_step = sip_step,
                 sip_count = sip_count,
                 dip = dip,
                 dip_value_type = dip_value_type,
                 dip_step = dip_step,
                 dip_count = dip_count)

    def _add_ipv6(self, append_to_stack='ethernet',
                 sip= "1.1.1.1",
                 sip_value_type = "singleValue",
                 sip_step = "0.0.0.1",
                 sip_count = 1,
                 dip = "2.2.2.2",
                 dip_value_type = "singleValue",
                 dip_step = "0.0.0.1",
                 dip_count = 1):
        '''
        internal method to add ipv6 header (can append to custom stack)
        '''
        outerIpv4FieldObj = self._createPacketHeader(self.traffic_item, packetHeaderToAdd=self.protocol['ipv6'], appendToStack=self.protocol[append_to_stack])

        outerIpv4FieldObj.find(DisplayName='Source Address').ValueType = sip_value_type
        outerIpv4FieldObj.find(DisplayName='Source Address').SingleValue = sip
        outerIpv4FieldObj.find(DisplayName='Source Address').StartValue = sip
        outerIpv4FieldObj.find(DisplayName='Source Address').StepValue = sip_step
        outerIpv4FieldObj.find(DisplayName='Source Address').CountValue = sip_count

        outerIpv4FieldObj.find(DisplayName='Destination Address').ValueType = dip_value_type
        outerIpv4FieldObj.find(DisplayName='Destination Address').SingleValue = dip
        outerIpv4FieldObj.find(DisplayName='Destination Address').StartValue = dip
        outerIpv4FieldObj.find(DisplayName='Destination Address').StepValue = dip_step
        outerIpv4FieldObj.find(DisplayName='Destination Address').CountValue = dip_count

    def add_ipv6(self,
                 after_vlan = False,
                 sip = "2001:0:0:0:0:0:0:1",
                 sip_value_type = "singleValue",
                 sip_step = "0:0:0:0:0:0:0:1",
                 sip_count = 1,
                 dip = "2002:0:0:0:0:0:0:2",
                 dip_value_type = "singleValue",
                 dip_step = "0:0:0:0:0:0:0:1",
                 dip_count = 1):
        '''
            stream method to add ipv6 header
                after_vlan: <bool> True if append after vlan tag, False if append after Ethrenet header
                sip: <string> first source ip in x:x:x:x:x:x:x:x format
                sip_step: <string> source ip step value in x:x:x:x:x:x:x:x format
                sip_count = <int> source ip count (ignored if value type is singleValue)
                sip_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dip: <string> first destination ip in x:x:x:x:x:x:x:x format
                dip_step: <string> destination ip step value in x:x:x:x:x:x:x:x format
                dip_count = <int> destination ip count (ignored if value type is singleValue)
                dip_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''
        if not after_vlan:
            stack = 'ethernet'
        else:
            stack = 'vlan'

        self._add_ipv6(append_to_stack=stack,
                 sip = sip,
                 sip_value_type = sip_value_type,
                 sip_step = sip_step,
                 sip_count = sip_count,
                 dip = dip,
                 dip_value_type = dip_value_type,
                 dip_step = dip_step,
                 dip_count = dip_count)

    def _add_tcp(self, append_to_stack='ipv4',
                 sport = 10000,
                 sport_value_type = "singleValue",
                 sport_step = 1,
                 sport_count = 1,
                 dport = 20000,
                 dport_value_type = "singleValue",
                 dport_step = 1,
                 dport_count = 1):
        '''
        internal method to add tcp header (can append to custom stack)
        '''
        tcpFieldObj = self._createPacketHeader(self.traffic_item, packetHeaderToAdd='^TCP', appendToStack=self.protocol[append_to_stack])
        tcpFieldObj.find(DisplayName='TCP-Source-Port').Auto = False
        tcpFieldObj.find(DisplayName='TCP-Source-Port').ValueType = sport_value_type
        tcpFieldObj.find(DisplayName='TCP-Source-Port').SingleValue = sport
        tcpFieldObj.find(DisplayName='TCP-Source-Port').StartValue = sport
        tcpFieldObj.find(DisplayName='TCP-Source-Port').StepValue = sport_step
        tcpFieldObj.find(DisplayName='TCP-Source-Port').CountValue = sport_count
        tcpFieldObj.find(DisplayName='TCP-Dest-Port').Auto = False
        tcpFieldObj.find(DisplayName='TCP-Dest-Port').ValueType = dport_value_type
        tcpFieldObj.find(DisplayName='TCP-Dest-Port').SingleValue = dport
        tcpFieldObj.find(DisplayName='TCP-Dest-Port').StartValue = dport
        tcpFieldObj.find(DisplayName='TCP-Dest-Port').StepValue = dport_step
        tcpFieldObj.find(DisplayName='TCP-Dest-Port').CountValue = dport_count


    def add_tcpv4(self,
                 sport = 10000,
                 sport_value_type = "singleValue",
                 sport_step = 1,
                 sport_count = 1,
                 dport = 20000,
                 dport_value_type = "singleValue",
                 dport_step = 1,
                 dport_count = 1):
        '''
            stream method to add tcpv4 header
                sport: <int> first source port
                sport_step: <int> source port step value
                sport_count = <int> source port count (ignored if value type is singleValue)
                sport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dport: <int> first destination port
                dport_step: <int> destination port step value
                dport_count = <int> destination port
                dport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''

        self._add_tcp(append_to_stack='ipv4',
                 sport = sport,
                 sport_value_type = sport_value_type,
                 sport_step = sport_step,
                 sport_count = sport_count,
                 dport = dport,
                 dport_value_type = dport_value_type,
                 dport_step = dport_step,
                 dport_count = dport_count)

    def add_tcpv6(self,
                 sport = 10000,
                 sport_value_type = "singleValue",
                 sport_step = 1,
                 sport_count = 1,
                 dport = 20000,
                 dport_value_type = "singleValue",
                 dport_step = 1,
                 dport_count = 1):
        '''
            stream method to add tcpv6 header
                sport: <int> first source port
                sport_step: <int> source port step value
                sport_count = <int> source port count (ignored if value type is singleValue)
                sport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dport: <int> first destination port
                dport_step: <int> destination port step value
                dport_count = <int> destination port
                dport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''

        self._add_tcp(append_to_stack='ipv6',
                 sport = sport,
                 sport_value_type = sport_value_type,
                 sport_step = sport_step,
                 sport_count = sport_count,
                 dport = dport,
                 dport_value_type = dport_value_type,
                 dport_step = dport_step,
                 dport_count = dport_count)

    def _add_udp(self, append_to_stack='ipv4',
                 sport = 10000,
                 sport_value_type = "singleValue",
                 sport_step = 1,
                 sport_count = 1,
                 dport = 20000,
                 dport_value_type = "singleValue",
                 dport_step = 1,
                 dport_count = 1):
        '''
        internal method to add udp header (can append to custom stack)
        '''
        udpFieldObj = self._createPacketHeader(self.traffic_item, packetHeaderToAdd='^UDP', appendToStack=self.protocol[append_to_stack])
        udpFieldObj.find(DisplayName='UDP-Source-Port').Auto = False
        udpFieldObj.find(DisplayName='UDP-Source-Port').ValueType = sport_value_type
        udpFieldObj.find(DisplayName='UDP-Source-Port').SingleValue = sport
        udpFieldObj.find(DisplayName='UDP-Source-Port').StartValue = sport
        udpFieldObj.find(DisplayName='UDP-Source-Port').StepValue = sport_step
        udpFieldObj.find(DisplayName='UDP-Source-Port').CountValue = sport_count
        udpFieldObj.find(DisplayName='UDP-Dest-Port').Auto = False
        udpFieldObj.find(DisplayName='UDP-Dest-Port').ValueType = dport_value_type
        udpFieldObj.find(DisplayName='UDP-Dest-Port').SingleValue = dport
        udpFieldObj.find(DisplayName='UDP-Dest-Port').StartValue = dport
        udpFieldObj.find(DisplayName='UDP-Dest-Port').StepValue = dport_step
        udpFieldObj.find(DisplayName='UDP-Dest-Port').CountValue = dport_count


    def add_udpv4(self,
                 sport = 10000,
                 sport_value_type = "singleValue",
                 sport_step = 1,
                 sport_count = 1,
                 dport = 20000,
                 dport_value_type = "singleValue",
                 dport_step = 1,
                 dport_count = 1):
        '''
            stream method to add udpv4 header
                sport: <int> first source port
                sport_step: <int> source port step value
                sport_count = <int> source port count (ignored if value type is singleValue)
                sport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dport: <int> first destination port
                dport_step: <int> destination port step value
                dport_count = <int> destination port
                dport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''

        self._add_udp(append_to_stack='ipv4',
                 sport = sport,
                 sport_value_type = sport_value_type,
                 sport_step = sport_step,
                 sport_count = sport_count,
                 dport = dport,
                 dport_value_type = dport_value_type,
                 dport_step = dport_step,
                 dport_count = dport_count)

    def add_udpv6(self,
                 sport = 10000,
                 sport_value_type = "singleValue",
                 sport_step = 1,
                 sport_count = 1,
                 dport = 20000,
                 dport_value_type = "singleValue",
                 dport_step = 1,
                 dport_count = 1):
        '''
            stream method to add udpv6 header
                sport: <int> first source port
                sport_step: <int> source port step value
                sport_count = <int> source port count (ignored if value type is singleValue)
                sport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
                dport: <int> first destination port
                dport_step: <int> destination port step value
                dport_count = <int> destination port
                dport_value_type = <string> (decrement|increment|nonRepeatableRandom|random|repeatableRandomRange|singleValue|valueList)
        '''

        self._add_udp(append_to_stack='ipv6',
                 sport = sport,
                 sport_value_type = sport_value_type,
                 sport_step = sport_step,
                 sport_count = sport_count,
                 dport = dport,
                 dport_value_type = dport_value_type,
                 dport_step = dport_step,
                 dport_count = dport_count)

    def _add_pfc_pause(self,
                       source_address_value_type=None,
                       source_address=None,
                       source_address_step_value=None,
                       source_address_count_value=None,
                       control_opcode_value_type=None,
                       control_opcode=None,
                       control_opcode_step_value=None,
                       control_opcode_count_value=None,
                       priority_enable_vector_value_type=None,
                       priority_enable_vector=None,
                       priority_enable_vector_step_value=None,
                       priority_enable_vector_count_value=None,
                       pfc_queue_0=None,
                       pfc_queue_1=None,
                       pfc_queue_2=None,
                       pfc_queue_3=None,
                       pfc_queue_4=None,
                       pfc_queue_5=None,
                       pfc_queue_6=None,
                       pfc_queue_7=None,
                       append_to_stack=None):
        """
        Internal method to add pfc pause header (can append to custom stack)
        """
        self.pfcPauseFieldObj = self._createPacketHeader(
            self.traffic_item,
            packetHeaderToAdd=self.protocol['pfcpause'],
            appendToStack=self.protocol[append_to_stack])

        #Set source address
        self.pfcPauseFieldObj.find(
            DisplayName='Source address').ValueType = source_address_value_type
        self.pfcPauseFieldObj.find(DisplayName='Source address').SingleValue = source_address
        self.pfcPauseFieldObj.find(DisplayName='Source address').StartValue = source_address
        self.pfcPauseFieldObj.find(
            DisplayName='Source address').StepValue = source_address_step_value
        self.pfcPauseFieldObj.find(
            DisplayName='Source address').CountValue = source_address_count_value

        #Set control opcode
        self.pfcPauseFieldObj.find(
            DisplayName='Control opcode').ValueType = control_opcode_value_type
        self.pfcPauseFieldObj.find(
            DisplayName='Control opcode').SingleValue = control_opcode
        self.pfcPauseFieldObj.find(
            DisplayName='Control opcode').StartValue = control_opcode
        self.pfcPauseFieldObj.find(
            DisplayName='Control opcode').StepValue = control_opcode_step_value
        self.pfcPauseFieldObj.find(
            DisplayName='Control opcode').CountValue = control_opcode_count_value

        #Set priority enable vector
        self.pfcPauseFieldObj.find(
            DisplayName='priority_enable_vector').ValueType = priority_enable_vector_value_type
        self.pfcPauseFieldObj.find(
            DisplayName='priority_enable_vector').SingleValue = priority_enable_vector
        self.pfcPauseFieldObj.find(
            DisplayName='priority_enable_vector').StartValue = priority_enable_vector
        self.pfcPauseFieldObj.find(
            DisplayName='priority_enable_vector').StepValue = priority_enable_vector_step_value
        self.pfcPauseFieldObj.find(
            DisplayName='priority_enable_vector').CountValue = priority_enable_vector_count_value

        #Set pause quanta
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 0').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 0').SingleValue = pfc_queue_0
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 1').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 1').SingleValue = pfc_queue_1
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 2').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 2').SingleValue = pfc_queue_2
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 3').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 3').SingleValue = pfc_queue_3
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 4').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 4').SingleValue = pfc_queue_4
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 5').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 5').SingleValue = pfc_queue_5
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 6').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 6').SingleValue = pfc_queue_6
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 7').ValueType = 'singleValue'
        self.pfcPauseFieldObj.find(
            DisplayName='PFC Queue 7').SingleValue = pfc_queue_7

    def add_pfc_pause(self,
                      source_address_value_type=None,
                      source_address=None,
                      source_address_step_value=None,
                      source_address_count_value=None,
                      control_opcode_value_type=None,
                      control_opcode=None,
                      control_opcode_step_value=None,
                      control_opcode_count_value=None,
                      priority_enable_vector_value_type=None,
                      priority_enable_vector=None,
                      priority_enable_vector_step_value=None,
                      priority_enable_vector_count_value=None,
                      pfc_queue_0=None,
                      pfc_queue_1=None,
                      pfc_queue_2=None,
                      pfc_queue_3=None,
                      pfc_queue_4=None,
                      pfc_queue_5=None,
                      pfc_queue_6=None,
                      pfc_queue_7=None,
                      append_to_stack=None,
                      after_vlan=None,
                      logical=True):
        """
        Stream method to add pfc pause header
        """
        source_address_value_type = source_address_value_type
        source_address = source_address
        source_address_step_value = source_address_step_value
        source_address_count_value = source_address_count_value
        control_opcode_value_type = control_opcode_value_type
        control_opcode = control_opcode
        control_opcode_step_value = control_opcode_step_value
        control_opcode_count_value = control_opcode_count_value
        priority_enable_vector_value_type = priority_enable_vector_value_type
        priority_enable_vector = priority_enable_vector
        priority_enable_vector_step_value = priority_enable_vector_step_value
        priority_enable_vector_count_value = priority_enable_vector_count_value
        pfc_queue_0 = pfc_queue_0
        pfc_queue_1 = pfc_queue_1
        pfc_queue_2 = pfc_queue_2
        pfc_queue_3 = pfc_queue_3
        pfc_queue_4 = pfc_queue_4
        pfc_queue_5 = pfc_queue_5
        pfc_queue_6 = pfc_queue_6
        pfc_queue_7 = pfc_queue_7
        after_vlan = after_vlan

        # Selecting stack
        if not after_vlan:
            stack = "ethernet"
        else:
            stack = "vlan"

        self._add_pfc_pause(source_address_value_type=source_address_value_type,
                            source_address=source_address,
                            source_address_step_value=source_address_step_value,
                            source_address_count_value=source_address_count_value,
                            control_opcode_value_type=control_opcode_value_type,
                            control_opcode=control_opcode,
                            control_opcode_step_value=control_opcode_step_value,
                            control_opcode_count_value=control_opcode_count_value,
                            priority_enable_vector_value_type=priority_enable_vector_value_type,
                            priority_enable_vector=priority_enable_vector,
                            priority_enable_vector_step_value=priority_enable_vector_step_value,
                            priority_enable_vector_count_value=priority_enable_vector_count_value,
                            pfc_queue_0=pfc_queue_0,
                            pfc_queue_1=pfc_queue_1,
                            pfc_queue_2=pfc_queue_2,
                            pfc_queue_3=pfc_queue_3,
                            pfc_queue_4=pfc_queue_4,
                            pfc_queue_5=pfc_queue_5,
                            pfc_queue_6=pfc_queue_6,
                            pfc_queue_7=pfc_queue_7,
                            append_to_stack=stack)
class Stats(object):
    '''
       Traffic Generator Stats object
       Attributes:
       self.ixnetwork: IxNetwork object
       self.traffic_item_stats_view: Stat view assistant object for Traffic Item Statistics page
       self.traffic_item_stats: Parsed polled output of the traffic item / stream stats
    '''
    def __init__(self, ixnetwork):
        self.ixnetwork = ixnetwork
        self.traffic_item_stats_view = StatViewAssistant(self.ixnetwork, 'Traffic Item Statistics')
        self.traffic_item_stats = {}

    def poll(self):
        '''
        Stats method to poll / refresh the stats for all of the streams / traffic items
        '''
        for row in range(0,len(self.traffic_item_stats_view.Rows)):
            flow_name = self.traffic_item_stats_view.Rows[row]['Traffic Item']
            self.traffic_item_stats[flow_name] = {}
            for column, stat in zip(self.traffic_item_stats_view.Rows.Columns, self.traffic_item_stats_view.Rows.RawData[row]):
                self.traffic_item_stats[flow_name][column] = stat
        return self.traffic_item_stats

    def get_tx_packet_count(self, flow_name):
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Tx Frames']))

    def get_rx_packet_count(self, flow_name):
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Rx Frames']))

    def get_tx_rate_bps(self, flow_name):
        '''
        Stats helper method to get the tx rate (bps) based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Tx Rate (bps)']))

    def get_rx_rate_bps(self, flow_name):
        '''
        Stats helper method to get the rx rate (bps) based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Rx Rate (bps)']))

    def get_tx_rate_kbps(self, flow_name):
        '''
        Stats helper method to get the tx rate (kbps) based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Tx Rate (Kbps)']))

    def get_rx_rate_kbps(self, flow_name):
        '''
        Stats helper method to get the rx rate (kbps) based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Rx Rate (Kbps)']))
    def get_tx_rate_l1(self, flow_name):
        '''
        Stats helper method to get the raw L1 tx rate (bps) based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Tx L1 Rate (bps)']))

    def get_rx_rate_l1(self, flow_name):
        '''
        Stats helper method to get the raw L1 rx rate (bps) based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Rx L1 Rate (bps)']))

    def get_loss_percentage(self, flow_name):
        '''
        Stats helper method to get the loss percentage  based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Loss %']))

    def get_tx_frames(self, flow_name):
        '''
        Stats helper method to get the tx frames based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Tx Frames']))

    def get_rx_frames(self, flow_name):
        '''
        Stats helper method to get the rx frames based on stream name
        '''
        self.poll()
        return int(float(self.traffic_item_stats[flow_name]['Rx Frames']))

    def verify_no_drop (self, flow_name=[], threshold=0, exclude_list=[]):
        '''
        Stats helper method to verify no drop on streams
        flow_name : <string list> List of traffic item to verify no drop.  If empty list all streams will be verified
        threshold : Declare there is drop if loss percentage > loss percentage
        exclude_list : <string list> LIst of traffic item to exclude from check

        Return Value:  True if there is drop on any stream
                       Flase if there is no drop on all of the checked stream
        '''
        self.poll()
        failed = False
        if flow_name != []:
            return(float(self.traffic_item_stats[flow_name]['Loss %']) <= threshold )
        else:
            for flow in self.traffic_item_stats.keys():
                if flow in exclude_list:
                    continue
                if not float(self.traffic_item_stats[flow]['Loss %']) <= threshold:
                    print ("Traffic flow %s has traffic loss %s %s" % (flow, self.traffic_item_stats[flow]['Loss %'], "%"))
                    failed = True
        return failed

    def verify_same_tx_rx_rate (self, flow_name=[], threshold=0.01, exclude_list=[]):
        '''
        Stats helper method to verify tx and rx rates are the same the stream
        flow_name : <string list> List of traffic item to verify tx rx rate.  If empty list all streams will be verified
        threshold : Declare tx rx rate not match if abs(tx_rate - rx_rate) / tx_rate > threshold
        exclude_list : <string list> LIst of traffic item to exclude from check

        Return Value:  True if there is tx rx rate does not match on any stream
                       Flase if there is tx rx rate matches on all of the checked stream
        '''
        self.poll()
        failed = False
        if flow_name != []:
            tx_rate = float(self.traffic_item_stats[flow_name]['Tx Rate (bps)'])
            rx_rate = float(self.traffic_item_stats[flow_name]['Rx Rate (bps)'])
            if abs(tx_rate - rx_rate) / tx_rate > threshold:
                print ("Traffic flow %s has different tx/rx rate: tx=%f (bps) rx=%f (bps) threshold=%f" % (flow_name, tx_rate, rx_rate, threshold))
                failed = True
        else:
            for flow in self.traffic_item_stats.keys():
                if flow in exclude_list:
                    continue
                tx_rate = float(self.traffic_item_stats[flow]['Tx Rate (bps)'])
                rx_rate = float(self.traffic_item_stats[flow]['Rx Rate (bps)'])
                if abs(tx_rate - rx_rate) / tx_rate > threshold:
                    print ("Traffic flow %s has different tx/rx rate: tx=%f (bps) rx=%f (bps) threshold=%f" % (flow_name, tx_rate, rx_rate, threshold))
                    failed = True
        return failed
