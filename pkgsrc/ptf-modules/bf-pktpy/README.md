# bf-pktpy
Barefoot Python Packet Generator (aka bf-pktpy)

## Requirements
__Note__: Intel recommends to use Ubuntu 18.04.

* Python v2.7.X (bf-pktpy is compatible with Python v3.5+)

* PIP (preferred installer program) v9.0.1+


### Install pre-requirements (for Python v2.7.X)
On debian based systems:
```text
sudo apt-get -y update

sudo apt-get -y install python-pip python-setuptools git python-lxml python-dev
```

The Python requirements are in the `requirements.txt`, to install it on the
 system, type:
 
```text
pip install -r requirements.txt
```


### Install pre-requirements (for Python v3.5+)
On debian based systems:
```text
sudo apt-get -y update

sudo apt-get -y install python3-pip python3-setuptools git python3-lxml python3-dev
```

To install pip:

```text
pip install --upgrade pip>=9.0.3
```

```text
sudo apt install python-pip
```

The Python requirements are in the `requirements.txt`, to install it on the
 system, type:
 
```text
pip install -r requirements.txt
```


## Build
To build a package, go to the root directory and type in the command
 line. The `wheel` package is necessary to execute this action. 

```text
python setup.py sdist bdist_wheel
```

## Install
To install a bf-pktpy, you can use a PIP. Type in the command line:

```text
pip install bf-pktpy*  
```


## bf-pktpy and PTF
Bf-pktpy can be used with PTF  (https://github.com/p4lang/ptf). In order to 
use them together, please install PTF (using pip). One can also use 
requirements file:

```text
pip install -r requirements-ptf.txt
```

What is more, when running PTF script, one needs to add additional flag 
`-pmm` with module name:

```text
ptf <other parameters> -pmm bf_pktpy.ptf.packet_pktpy
```
Please make sure that `bf_pktpy` is loaded into the runtime before running 
PTF script!


### Available packets
All available packets you can import from `bf_pktpy.packets`.

#### How to import 
Example for import the Ether:
```python
from bf_pktpy.packets import Ether
```

#### Available packets:
* ARP
* BFD
* BOOTP 
* DHCP 
* Ether (Ethernet) 
* ERSPAN (II & III)
* GRE 
* ICMP
* IPv4
* IPv6
* MPLS 
* TCP 
* UDP
* VXLAN

#### And custom from bf-switch (some of them are in the WIP):
* DTEL_report_header
* fabric_cpu_header
* fabric_cpu_timestamp_header
* fabric_header
* fabric_payload_header
* mod_header
* postcard_header

### Available commands
| command name | command description |
|--------------|--------------------|
|send | Send packet at layer 3 |
|sendp | Send packet at layer 2 |
|sr | Send and receive packet at layer 3 |
|sr1 | Send and receive packet at layer 3. Return only 1st answer |
|srp | Send and receive packet at layer 2 |
|srp1 | Send and receive packet at layer 3. Return only 1st answer |
|srloop | Send and receive packets in loop at layer 2 | 
|srploop | Send and receive packets in loop at layer 3 |
|sniff | |
|bridge_and_sniff | | 


### Additional functionality
This section stores information about additional features and similarities to the Scapy.

#### load_bytes `pkt.__class__(packet_bytes)`
Load bytes allows to load a received bytes into a structure (If we know the packets that made up the frame).

Example:
```python
pkt = Ether() / IP() / TCP()
new_packet = pkt.load_bytes(bytes(pkt))  # value: bytes array 
# new_packet:  New Packet (Base) Object
```

Regarding p4-tests, `tna_timestamp`.
Let's define a simple udp packet.
```python
ipkt_payload = struct.pack("I", 0) * 10
ipkt = testutils.simple_udp_packet(eth_dst='11:11:11:11:11:11',
                                   eth_src='22:22:22:22:22:22',
                                   ip_src='1.2.3.4',
                                   ip_dst='100.99.98.97',
                                   ip_id=101,
                                   ip_ttl=64,
                                   udp_sport=0x1234,
                                   udp_dport=0xabcd,
                                   with_udp_chksum=False,
                                   udp_payload=ipkt_payload)

testutils.send_packet(self, swports[0], ipkt)
```

When we receive packet:
```python
(_, _, rcv_pkt, _) = testutils.dp_poll(self, 0, swports[1], timeout=2)

# previous implementation
nrcv = ipkt.__class__(rcv_pkt)

# new implementation 
nrcv = ipkt.load_bytes(rcv_pkt)

```

## New Packet class
To create a new version of packet you need to inherit from the Packet class 
(similar to other popular python's solution -- Scapy).
The basic structure is similar to the Scapy packet implementation standards and simple packages should be cross-compatible.

Example of Ethernet class:

```python
from bf_pktpy.library.specs.packet import Packet
from bf_pktpy.library.helpers.ether_types import ETYPES
from bf_pktpy.library.fields import (
    DestMACField,
    SourceMACField,
    XShortEnumField,
)


class Ether(Packet):
    name = "Ethernet"
    fields_desc = [
        DestMACField("dst"),
        SourceMACField("src"),
        XShortEnumField("type", 0x9000, types=ETYPES),
    ]
```

## Test profiling
To view files created by cProfile you need for example cprofilev library. 
Files are being stored as artifacts from Jenkins pipeline. To view them
execute command `python -m cprofilev -f combined.prof` and go to local 
server for a html preview of results.