# BMAI in a Folded Pipeline

## Scope
The goal of this document is to describe the usage of BMAI API and switch.p4 in a folded pipeline scenario. It will describe best practices when adding new P4 tables, writing application code to program these tables and how to in general maintain the codebase to flexibly serve both folded and symmetric pipeline configurations.

## Tofino pipe configuration
Tofino 1/2/3 have from 1-4 pipes in their setup based on the chip SKU. Each pipe typically has upto 16 ports. For a standard configuration of 4 pipes, the driver by default treats them to be operating in symmetric mode. What this means, the P4 program is copied to all pipes and any table configuration is replicated to all pipes in an opaque manner.
```
Pipeline <ipipe, epipe> () pipe;
Switch (pipe) main;
                 +-------+
    ----ipipe----|       |----epipe----
    ----ipipe----|  TM   |----epipe----
    ----ipipe----|       |----epipe----
    ----ipipe----|       |----epipe----
                 +-------+ 
Here the packet path is from_wire -> ipipe -> epipe -> to_wire
The forwarding decision will allow the TM to forward the packet to a pipe different from the pipe the packet ingressed on.
```
Folded pipe configuration is a different supported configuration where pipes are concatenated. The advantage of this configuration is a P4 program has a lot more stages to work with and allows for a much higher table scale. The downside to this is increased latency among others.
```
Pipeline <ip0, ep0> () pipe0;
Pipeline <ip1, ep1> () pipe1;
Pipeline <ip2, ep2> () pipe2;
Pipeline <ip3, ep3> () pipe3;
Switch (pipe0, pipe1, pipe2, pipe3) main;
               +-------+
    ----ip0----|       |----ep0----
    ----ip1----|  TM   |----ep1----
    ----ip2----|       |----ep2----
    ----ip3----|       |----ep3----
               +-------+
In this case, pipe0 is the pipe having external ports. This configuration is called 16Q. The internal pipe ports basically are configured as loopback automatically by the driver.

Here the packet path is from_wire -> ip0 -> ep1 -> ip1 -> ep2 -> ip2 -> ep3 -> ip3 -> ep0 -> to_wire
```

## P4 code organization
The P4 code is organized like a shared library. The individual dataplane modules are maintained in their own files grouped as features. For example, route related modules are in l3.p4 while QoS related modules go into qos.p4. They can be found at p4src/shared/*.p4.

These modules are then grouped together in top-level files to create a profile. The profiles are created per Tofino SKU and organized in SKU specific directories at p4src/switch-tofino/ or p4src/switch-tofino2 and so on. The contents of each profile is out of the scope of this document.

The P4-16 specification treats modules as control blocks. Each control block is analogous to a C++ class and each block can be instantiated any number of times just like a C++ object.
The *Nexthop* control block defined below can be instantiated twice like this.
```
Nexthop(NEXTHOP_TABLE_SIZE, ECMP_GROUP_TABLE_SIZE, ECMP_SELECT_TABLE_SIZE) nexthop1;
Nexthop(NEXTHOP_TABLE_SIZE, ECMP_GROUP_TABLE_SIZE, ECMP_SELECT_TABLE_SIZE) nexthop2;
```
These control block definitions play a major role in how API code is written today.

## switch.p4
switch.p4 is the colloquial name given to the P4 program maintained in BMAI.

Each profile has the pipeline defined with 6 major control blocks in switch.p4.

```cpp
Pipeline <switch_header_t, switch_local_metadata_t, switch_header_t, switch_local_metadata_t> (
    SwitchIngressParser(),
    SwitchIngress(),
    SwitchIngressDeparser(),
    SwitchEgressParser(),
    SwitchEgress(),
    SwitchEgressDeparser()) pipe;
Switch(pipe) main;
```

SwitchIngressParser, SwitchIngressDeparser, SwitchEgressParser, SwitchEgressDeparser are parser and deparser control blocks and are common across all switch profiles.
SwitchIngress and SwitchEgress are the specific control blocks which are unique for each switch profile.

```cpp
control SwitchIngress(
        inout switch_header_t hdr,
        inout switch_local_metadata_t local_md,
        in ingress_intrinsic_metadata_t ig_intr_md,
        in ingress_intrinsic_metadata_from_parser_t ig_intr_from_prsr,
        inout ingress_intrinsic_metadata_for_deparser_t ig_intr_md_for_dprsr,
        inout ingress_intrinsic_metadata_for_tm_t ig_intr_md_for_tm) {
    ...
    Nexthop(NEXTHOP_TABLE_SIZE, ECMP_GROUP_TABLE_SIZE, ECMP_SELECT_TABLE_SIZE) nexthop;  // instantiation
    ...
    ...

    ...
    nexthop.apply(local_md);  // invocation
    ...
    ...
}
```
SwitchIngress is a collection of control blocks which instantiate more control blocks from other files.

```cpp
control Nexthop(inout switch_local_metadata_t local_md) {  // definition
    action set_rif(rif_id_t rif) {
        ...
    }
    table nexthop {
        key = {
            local_md.lookup.ipv4_addr : exact;
        }
        ...
    }
    table ecmp {
        key = {
            local_md.lookup.ipv4_addr : exact;
        }
        ...
    }
    apply {
        switch(nexthop.apply().action_run) {
            NoAction : { ecmp.apply(); }
            default : {}
        }
    }
}
```

The P4C compiler generates the table names with a fully qualified namespace in a file referred to as bf-rt.json. So the name assigned to the nexthop and ecmp tables defined in the *Nexthop* control block look like this.
* pipe.SwitchIngress.nexthop.nexthop
* pipe.SwitchIngress.nexthop.ecmp

Similarly, the name assigned to the set_rif() action is
* pipe.SwitchIngress.nexthop.set_rif

## API BF-RT IDS
The API code refers to these tables using integer values generated again by the compiler and stored in bf-rt.json. The driver intialization reads this JSON and creates a map of the table name string to the table IDs in a map called bf_rt_info. The switch intialiazation logic reads this map and maintains a static data structure of the IDs of every table, action, match_key, action_parameters, etc. This logic is present in api/switch_tna/bf_rt_ids.cpp.
* T_NEXTHOP_TABLE = table_id_from_name("pipe.SwitchIngress.nexthop.nexthop")
* T_ECMP_TABLE = table_id_from_name("pipe.SwitchIngress.nexthop.ecmp")

## Historial limitation
BMAI is written targetted for standard pipeline configuration. Standard here means symmetric configuration. The SwitchIngress and SwitchEgress control blocks only existed in the pipes with the front panel ports. switch.p4 is also written within this paradigm that the P4 tables will always exist within these control blocks. For folded pipe configuration, the internal pipes were left to be managed by a user application and BMAI had no control on these pipes.

The following example why these tables are limited to external pipes only.
```cpp
Pipeline <switch_header_t, switch_local_metadata_t, switch_header_t, switch_local_metadata_t> (
    SwitchIngressParser(),
    CustomerIngress(),
    SwitchIngressDeparser(),
    SwitchEgressParser(),
    CustomerEgress(),
    SwitchEgressDeparser()) pipe;

Pipeline <ip0, ep0> () pipe;
Pipeline <ip1, ep1> () customer_pipe;
Switch (pipe, customer_pipe) main;
```
In this example, assume the user is using Tofino in a 32Q configuration and the user would like to reuse the above Nexthop control block in the CustomerIngress control block instead of the standard SwitchIngress. The bf-rt.json generated by the compiler for this table would look like below.
* customer_pipe.CustomerIngress.nexthop.nexthop

It becomes quickly obvious that the switch API code will fail in the "table_id_from_name()" API call because the implementation hard codes everything to exist in the SwitchIngress block.

## Coding Guidelines
```cpp
control SMAC(in mac_addr_t src_addr,
             inout switch_local_metadata_t local_md,
             inout switch_digest_type_t digest_type) {
    action smac_miss() { src_miss = true; }

    table smac {
        key = {
            local_md.bd : exact;
            src_addr : exact;
        }

        actions = {
            @defaultonly smac_miss;
        }
    }
}
```

### Table Names
All tables accessed in the API code must use an annotation.

@name(".smac") will generate the table name as "pipe.smac" in BFRT. The code in bf_rt_ids.cpp can then simply refer to this tables as 
```cpp
T_SMAC = table_id_from_name("smac");
```

The ID resolver is smart enough to figure out the pipe prefix associated with this table and prepend it with "pipe.".

### Action Names
The compiler exhibits the following behavior with various annotating schemes.

For global actions (defined outside a control block)
 - An annotation is not required and the name generated is "smac_miss" in BFRT

For local actions (defined in a control block)
 - If no annotation is specified, compiler generates "SwitchIngress.smac.smac_miss" in BFRT
 - Adding @name("smac_miss") will generate the name as "SwitchIngress.smac.smac_miss" in BFRT (same as above)
 - Adding @name(".smac_miss") will generate the name as "smac_miss" in BFRT

 The recommendation is to use 3rd option. The code in bf_rt_ids.cpp can then simply refer to this action as
```cpp
A_SMAC_MISS = action_id_from_name(T_SMAC, "smac_miss");
```
