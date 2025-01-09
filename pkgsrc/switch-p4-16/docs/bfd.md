# 1.0 **Introduction**

BFD (Bidirectional Forwarding Detection) protocol is used to determine the data plane connectivity between two forwarding elements with a low overhead and low detection time. In BFD operation, two forwarding elements first setup a BFD session between them and then exchange BFD hello packets at a specified interface. If a reply is not received withing a specified period of time, a failure is reported to higher routing layers so they can find an alternate path between these two forwarding lements.

BFD can be abstracted as a simple service.  The service primitives provided by BFD are to create, destroy, and modify a session, given the destination address and other parameters.  BFD in return provides a signal to its clients indicating when the BFD session goes up or down

## References

1. [RFC5880 - Bidirectional Forwarding Detection](https://datatracker.ietf.org/doc/html/rfc5880)
2. [SAI BFD Proposal](https://github.com/opencomputeproject/SAI/blob/master/doc/BFD/SAI_Change_Proposal_BFD_v0_2.docx)
3. [SONiC BFD HW Offload HLD](https://github.com/Azure/SONiC/blob/master/doc/bfd/BFD%20HW%20Offload%20HLD.md)
4. [BFD Suuport on ASR9K](https://community.cisco.com/t5/service-providers-documents/bfd-support-on-cisco-asr9000/ta-p/3153191)


# 2.0 **BFD Protocol Overview**

BFD is a simple Hello protocol, similar to most of the L3 routing protocols. BFD runs between a pair of systems exchanging BFD packets periodically and if a system stops receiving BFD packets for long enough time, some component in the path to the neighboring system is assumed to have failed.

A path is only declared to be operational when two-way communication has been established between two systems. A separate BFD session is created for each pair of systems and for each dataplane protocol

BFD control Packet

    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |Vers |  Diag   |Sta|P|F|C|A|D|M|  Detect Mult  |    Length     |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                       My Discriminator                        |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                      Your Discriminator                       |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                    Desired Min TX Interval                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                   Required Min RX Interval                    |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |                 Required Min Echo RX Interval                 |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Vers

          The version number of the protocol

  Diagnostic (Diag)

        A diagnostic code specifying the local system's reason for the last change in the session state. Values are:

            0 -- No Diagnostic
            1 -- Control Detection Time Expired
            2 -- Echo Function Failed
            3 -- Neighbor Signaled Session Down
            4 -- Forwarding Plane Reset
            5 -- Path Down
            6 -- Concatenated Path Down
            7 -- Administratively Down
            8 -- Reverse Concatenated Path Down
            9-31 -- Reserved for future use

        This field allows remote systems to determine the reason that the previous session failed, for example.

  State (sta)

    The current BFD session state as seen by the transmitting system.
      Values are:

         0 -- AdminDown
         1 -- Down
         2 -- Init
         3 -- Up

  Poll (P)

      If set, the transmitting system is requesting verification of connectivity, or of a parameter change, and is expecting a packet with the Final (F) bit in reply.  If clear, the transmitting
      system is not requesting verification.

  Final (F)

      If set, the transmitting system is responding to a received BFD Control packet that had the Poll (P) bit set.  If clear, the transmitting system is not responding to a Poll.

  Control Plane Independent (C)

      If set, the transmitting system's BFD implementation does not share fate with its control plane (in other words, BFD is implemented in the forwarding plane and can continue to function through disruptions in the control plane).  If clear, the transmitting system's BFD implementation shares fate with its control plane. SDE will transparenly pass this bit from the application to the BFD packets.


  Authentication Present (A)

      If set, the Authentication Section is present and the session is
      to be authenticated. Not supported.

  Demand (D)

      If set, Demand mode is active in the transmitting system (the system wishes to operate in Demand mode, knows that the session is Up in both directions, and is directing the remote system to cease the periodic transmission of BFD Control packets).  If clear, Demand mode is not active in the transmitting system. Not supported in SDE.

  Multipoint (M)

      TBD

  Detect Mult

      Detection time multiplier.  The negotiated transmit interval, multiplied by this value, provides the Detection Time for the receiving system in Asynchronous mode.

  Length

      Length of the BFD Control packet, in bytes.

  My Discriminator

      A unique, nonzero discriminator value generated by the
      transmitting system, used to demultiplex multiple BFD sessions
      between the same pair of systems.

  Your Discriminator

      The discriminator received from the corresponding remote system.
      This field reflects back the received value of My Discriminator,
      or is zero if that value is unknown.

  Desired Min TX Interval

      This is the minimum interval, in microseconds, that the local
      system would like to use when transmitting BFD Control packets,
      less any jitter applied (see section 6.8.2).  The value zero is
      reserved.

  Required Min RX Interval

      This is the minimum interval, in microseconds, between received
      BFD Control packets that this system is capable of supporting,
      less any jitter applied by the sender (see section 6.8.2).  If
      this value is zero, the transmitting system does not want the
      remote system to send any periodic BFD Control packets.

  Required Min Echo RX Interval

      This is the minimum interval, in microseconds, between received
      BFD Echo packets that this system is capable of supporting, less
      any jitter applied by the sender (see section 6.8.9).  If this
      value is zero, the transmitting system does not support the
      receipt of BFD Echo packets.

  Auth Type

      The authentication type in use, if the Authentication Present (A)
      bit is set.

         0 - Reserved
         1 - Simple Password
         2 - Keyed MD5
         3 - Meticulous Keyed MD5
         4 - Keyed SHA1
         5 - Meticulous Keyed SHA1
     6-255 - Reserved for future use

  Auth Len

      The length, in bytes, of the authentication section, including the
      Auth Type and Auth Len fields.


## 2.1 **BFD Operating Modes**

There are two modes where BFD can operate. And the system can be either Active or Passive.

Asynchronous: This is considered to be the primary mode in BFD, In this mode, both the systems send the BFD control packet to one another periodically. If a packet is not received from the other system for a long enough duration then the session is declared down

Demand Mode: As the name suggests there is no periodic packet exchange happens in this mode. But when the system feels to verify the connectivity a short sequence of BFD control packets are exchanged to verify connectivity. This is to reduce overhead.

Echo Function: Parallel to these modes, BFD support the Echo function, where the system generates a stream of bfd echo packets which is looped back by the other system. If the same number of those packets are not received then the session is declared down.

## 2.2 **BFD state machine**

The BFD state machine implements a three-way handshake, both when establishing a BFD session and when tearing it down for any reason, to ensure that both systems are aware of the state change.

## 2.3 **BFD HW Offload**

Typically when the BFD session begins, the systems operate at periodic slow exchange of control packets. When the bidirectional communication is achieved the BFD session become UP. At this point the system can choose to operate at a higher desired rate by negotiating the rate with it's neighbor. At this point the BFD application can choose to offload the session to the HW which can periodically send these BFD packets at faster rate, track the session state and inform the control plane of any state change events.


# 3.0 **Switch.p4 Support**

Programming pipeline of Tofino ASICs can be used to implement the HW offload portion of BFD protocol. In the proposed implementation, BMAI takes care of setting up and tearing down of BFD sessions and the job of peridically sending hellos and keeping track of session state is offloaded to P4 programmable pipeline in Tofino HW.

## 3.1 **Features/Scale**
- Up to 4096 BFD sessions
- Only async mode sessions can be offloaded.
- Tx and Rx negotiated timer values must be multiple of 250ms.
- Support for both IPv4 and IPv6
- Support both 1-hop and multi-hop BFD
- use regular forwarding pipeline for BFD Tx packets

## 3.2 **SAI Support**

|   SAI Attribute   |   SDE support   |
|---------------|-------------|
|     SAI_BFD_SESSION_ATTR_TYPE | Only ASYNCH_ACTIVE is supported
|     SAI_BFD_SESSION_ATTR_HW_LOOKUP_VALID   | Only True mode is supported
|     SAI_BFD_SESSION_ATTR_VIRTUAL_ROUTER   | Supported
|     SAI_BFD_SESSION_ATTR_PORT   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_LOCAL_DISCRIMINATOR   | Supported
|     SAI_BFD_SESSION_ATTR_REMOTE_DISCRIMINATOR   | Supported
|     SAI_BFD_SESSION_ATTR_UDP_SRC_PORT   | Supported
|     SAI_BFD_SESSION_ATTR_TC   | Supported
|     SAI_BFD_SESSION_ATTR_VLAN_TPID   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_VLAN_ID   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_VLAN_PRI   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_VLAN_CFI   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_VLAN_HEADER_VALID   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_BFD_ENCAPSULATION_TYPE   | Only "None" is supported
|     SAI_BFD_SESSION_ATTR_IPHDR_VERSION   | Supported
|     SAI_BFD_SESSION_ATTR_TOS   | Supported
|     SAI_BFD_SESSION_ATTR_TTL   | Supported
|     SAI_BFD_SESSION_ATTR_SRC_IP_ADDRESS   | Supported
|     SAI_BFD_SESSION_ATTR_DST_IP_ADDRESS   | Supported
|     SAI_BFD_SESSION_ATTR_TUNNEL_TOS   | Not Supported
|     SAI_BFD_SESSION_ATTR_TUNNEL_TTL   | Not Supported
|     SAI_BFD_SESSION_ATTR_TUNNEL_SRC_IP_ADDRESS   | Not Supported
|     SAI_BFD_SESSION_ATTR_TUNNEL_DST_IP_ADDRESS   | Not Supported
|     SAI_BFD_SESSION_ATTR_SRC_MAC_ADDRESS   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_DST_MAC_ADDRESS   | Not Supported and not required when HW_LOOKUP_VALID=true
|     SAI_BFD_SESSION_ATTR_ECHO_ENABLE   | Not Supported
|     SAI_BFD_SESSION_ATTR_MULTIHOP   | Supported
|     SAI_BFD_SESSION_ATTR_CBIT   | Not Supported
|     SAI_BFD_SESSION_ATTR_MIN_TX   | Supported
|     SAI_BFD_SESSION_ATTR_MIN_RX   | Supported
|     SAI_BFD_SESSION_ATTR_MULTIPLIER   | Supported
|     SAI_BFD_SESSION_ATTR_REMOTE_MIN_TX   | Supported
|     SAI_BFD_SESSION_ATTR_REMOTE_MIN_RX   | Supported
|     SAI_BFD_SESSION_ATTR_STATE   | Supported
|     SAI_BFD_SESSION_ATTR_OFFLOAD_TYPE   | Only None and full are supported for now
|     SAI_BFD_SESSION_ATTR_NEGOTIATED_TX   | Supported
|     SAI_BFD_SESSION_ATTR_NEGOTIATED_RX   | Supported
|     SAI_BFD_SESSION_ATTR_LOCAL_DIAG   | Not Supported ??
|     SAI_BFD_SESSION_ATTR_REMOTE_DIAG   | Not Supported ??
|     SAI_BFD_SESSION_ATTR_REMOTE_MULTIPLIER   | Supported

## 3.2 **Dataplane design - Summary**

**- PktGen**

    - Pktgen generates a BFD packet at a common denominator rate (250ms).
    - Pktgen packet is a trigger for sending BFD packet as well as detecting session failure.
    - BFD sessions are distibuted across all available pipelines. For each session, one of the pipes is selected as the designated pipe.
    - Both Rx and Tx timers for a session are handled on the designated pipe only.
    - N-packets are generated in one burst to service all N sessions per pipeline (TBD)
    - Pktgen packet is a BFD packet with a cpu header. Bit 15 of the reason_code in cpu header field has been commandeered to indicate a BFD-TX packet from PktGen and session information is encoded in the last 12-bits of the reason code.

**- BFD-Tx**

Triggered by a BFD packet received from PktGen

- Find active BFD session and retrieve session parameters (ingress)
  - If not offloaded/active - drop
  - Update destination IP address of the packet based on session info

- Check Rx timers (ingress)
  - If no packet was received in the negotiated rx timout, send packet to CPU with a reason_code and stop offload for that session.
  - Else, decrement the timer.

- Perform a regular routing lookup using the new destination IP (ingress)

- Check Tx timer (egress)
  - If Tx timer has expired, send the BFD packet else drop.
  - Modify BFD header fields with the correct session info


**- BFD-Rx**

Triggered by a BFD packet received on a front-panel or recirc port

- Find Rx session using session discriminators and other fields from the BFD header
  - If session is not found, send Rx packet to CPU
    - TODO : how to distinguish between sessions not configured from the sessions with changed parameters
- Rx packets can arrive on any port depending on the physical reachability of the peer, while a designated pipe is used to manage a give bfd-session
- If packet arrived on a non-designated pipe, recirculate the packet to the appropriate pipe for timer reset function.
- Reset the timer back to the rx_mult value for the session
  - rx_mult = (negoriated_rx_interval * detect_mult) / pktgen_timer
- Drop the packet after timer reset function is done


# 4 **Control Plane design**

The user application expects the SDE implementation to support session setup and teardown. Unlike other control plane applications like LACP, LLDP, etc where the applications have daemons to manage session state, BFD implementation requires the SDK to manage session state, packet forwarding/reception, FSM transitions, etc.

To support this control plane requirement, the following new software components are implemented.
1. New BFD daemon running in BMAI context (referred to as bfdd)
2. Tofino PKTGEN session management in BMAI
3. BMAI integration with bfdd
4. Session offload and onload to/from HW via switch packet driver
5. Packet driver enhancement to work with the new BFD daemon
6. API changes to support new BFD related P4 tables described in the data plane section above
7. Session shutdown notifications to SAI


## 4.1 Software Architecture
The new BFDD will run as a new thread and maintain its own state related to session management. BFDD will interface with the existing switch packet driver to send and receive packets.

    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |              SAI                  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |             BMAI                  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    |  API    |   BFDD    |    PKT DRV  |
    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

### 4.1.1 Session create
1. SAI receives a BFD session create call from application/SONiC
2. BMAI BFD object create call
3. Create a session in BFDD context
4. Program necessary P4 tables with new session

### 4.1.2 Session destroy
1. SAI receive BFD session delete
2. BMAI BFD session delete
3. BFDD sends session admindown packet to remote and closes session context
4. Remove entries from P4 tables

### 4.1.3 Session down
1. Dataplane determines session down and sends CPU packet
2. Packet driver receives packet and forward to bfdd
3. BFDD session context update and send notification to SAI
4. SAI invokes application callback with session down information

## 4.2 Packet path
BFD starts up in slow mode until a session is established at which point the session is offloaded to HW.

### BFD Rx packets
Dataplane -> CPU port/PCIE -> Packet driver -> BFDD

### BFD Tx packets
BFDD -> Packet driver -> CPU port/PCIE -> Dataplane


## 4.3 BFD Daemon
A new daemon BFDD(thread) will be created in BMAI context.
This daemon is initialized with below responsibilities
   1. Maintain the states of multiple BFD sessions and transition of states based on the peer state & local state.
   2. Interaction with packet driver for Rx/Tx of BFD pkts & offload/onload of sessions to/from HW.
   3. Interaction with SAI layer for sessions create/destroy/down

### 4.3.1 FSM state machine
BFD state machine shall be any one of the below states for each session. States shall be changed based on local and remote BFD state.
   1. init
   2. up
   3. down
   4. admin down
	
Init state means that the remote system is communicating.A session will remain in Init state until either a BFD Control Packet 
is received that is signaling Init or Up state (in which case the session advances to Up state) or the Detection Time expires, 
meaning that communication with the remote system has been lost (in which case the session advances to Down state).

Down state means that the session is down (or has just been created).A session remains in Down state until the remote system
indicates that it agrees that the session is down by sending a BFD Control packet with the State field set to anything other 
than Up.  If that packet signals Down state, the session advances to Init state; if that packet signals Init state, 
the session advances to Up state.
	
Up state means that the BFD session has successfully been established, and implies that connectivity between the systems is 
working.The session will remain in the Up state until either connectivity fails or the session is taken down administratively.
	
AdminDown state means that the session is being held administratively down.
	
                             +--+
                             |  | UP, ADMIN DOWN, TIMER
                             |  V
                     DOWN  +------+  INIT
              +------------|      |------------+
              |            | DOWN |            |
              |  +-------->|      |<--------+  |
              |  |         +------+         |  |
              |  |                          |  |
              |  |               ADMIN DOWN,|  |
              |  |ADMIN DOWN,          DOWN,|  |
              |  |TIMER                TIMER|  |
              V  |                          |  V
            +------+                      +------+
       +----|      |                      |      |----+
   DOWN|    | INIT |--------------------->|  UP  |    |INIT, UP
       +--->|      | INIT, UP             |      |<---+
            +------+                      +------+
			
	
### 4.3.2 Interface with SAI, PktGen & Packet Driver 
#### Session Create
Get the session create message from SAI 
Create a session in BFDD, update to hash table
Update P4 objects/tables appropriately 
Offload the Pkt Tx/Rx to PktGen for the session

#### Session Destroy 
Get the session destroy message from SAI
Delete session in BFDD, remove from hash table
Update  P4 objects/tables appropriately 
Delete pkt Tx/Rx from PktGen for the session

#### Session Down 
HW determines session down and send trap to CPU
Packet Driver receives the pkt and sends to BFDD
BFDD update session state & notify session down to SAI
SAI invokes concerned application with session down  

### 4.3.4 Data Structures used for handling multiple BFD sessions and states	
Should support 4096 BFD sessions
Create a hash table to store BFD sessions
Add to the table when new session created 
Delete from the table when session is deleted

### 4.3.5 Stats & debugging 
Capture below stats for each session & number of sessions handling 
  Session state, target IP, time interval, 
  Command to show this o/p [TBD]


## 4.4 BMAI object model

## 4.5 PKTGEN app ID and buffer management

## 4.6 Switch Packet driver enhancements

## Unit test plan
