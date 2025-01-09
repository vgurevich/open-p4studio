# Anatomy of SAI maps #

SAI maps are implementation helpers which ease converting SAI objects and attributes to corresponding BMAI data. It works by converting the JSON files in this directory into a CPP file after parsing the JSON into SAI enumerations.


To generate maps from JSON,
python3 s3/tools/sai_map_gen.py <this-dir> <out-dir>

sai_map_gen.py generates sai_map.cpp and sai_map.h in the build/sai directory which is then compiled into libsai.so.

Enams are mapped by short names.
For exemple to map "my_sid_entry" "packet_action" we need only map enums:
```json
{
  "my_sid_entry" : {
    ...
    "attributes" : {
      "packet_action" : "packet_action"
      ...
    }
  }
}
```
This json code will map next values:
```
SAI_PACKET_ACTION_DROP          ->  SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_DROP
SAI_PACKET_ACTION_FORWARD       ->  SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_FORWARD
SAI_PACKET_ACTION_COPY          ->
SAI_PACKET_ACTION_COPY_CANCEL   ->
SAI_PACKET_ACTION_TRAP          ->  SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_TRAP
SAI_PACKET_ACTION_LOG           ->  SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_DENY
SAI_PACKET_ACTION_DENY          ->
SAI_PACKET_ACTION_TRANSIT       ->  SWITCH_MY_SID_ENTRY_ATTR_PACKET_ACTION_TRANSIT
```

If we have different enum naming - we need to specify every enum value.
For exemple to map "hostif_table_entry" "type" and "channel_type" we need map enums and specify every enum value:
```json
    "hostif_table_entry" : {
        ...
        "switch" : "hostif_rx_filter",
        "attributes" : {
            ...
            "type" : "type",
            "channel_type" : "channel_type"
        },
        "enums" : {
            "type" : {
                "port" : "port",
                "lag" : "lag",
                "vlan" : "vlan",
                "wildcard" : "wildcard",
                "trap_id" : "trap"
            },
            "channel_type" : {
                "cb" : "cb",
                "fd" : "fd",
                "netdev_physical_port" : "port",
                "netdev_logical_port" : "lag",
                "genetlink" : "genetlink"
            }
        }
    }
}

```
This json code will map next values:

```
SAI_HOSTIF_TABLE_ENTRY_TYPE_PORT        ->  SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_PORT
SAI_HOSTIF_TABLE_ENTRY_TYPE_LAG         ->  SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_LAG
SAI_HOSTIF_TABLE_ENTRY_TYPE_VLAN        ->  SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_VLAN
SAI_HOSTIF_TABLE_ENTRY_TYPE_TRAP_ID     ->  SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_TRAP
                                        ->  SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_RIF
SAI_HOSTIF_TABLE_ENTRY_TYPE_WILDCARD    ->  SWITCH_HOSTIF_RX_FILTER_ATTR_TYPE_WILDCARD

SAI_HOSTIF_TABLE_ENTRY_CHANNEL_TYPE_CB                      ->  SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_CB
SAI_HOSTIF_TABLE_ENTRY_CHANNEL_TYPE_FD                      ->  SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_FD
SAI_HOSTIF_TABLE_ENTRY_CHANNEL_TYPE_NETDEV_PHYSICAL_PORT    ->  SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_PORT
SAI_HOSTIF_TABLE_ENTRY_CHANNEL_TYPE_NETDEV_LOGICAL_PORT     ->  SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_LAG
SAI_HOSTIF_TABLE_ENTRY_CHANNEL_TYPE_NETDEV_L3               ->
SAI_HOSTIF_TABLE_ENTRY_CHANNEL_TYPE_GENETLINK               ->  SWITCH_HOSTIF_RX_FILTER_ATTR_CHANNEL_TYPE_GENETLINK
```

If we have different enum naming and global sai enum - we need to specify every enum value and specify sai enum as global.
```json
{
   "policer" : {
       "profile" : "Per profile info",
       "switch" : "meter",
       "attributes" : {
           "mode" : "mode",
           "cbs" : "cbs",
           "cir" : "cir",
           "pbs" : "pbs",
           "pir" : "pir",
           "green_packet_action" : "green_packet_action",
           "red_packet_action" : "red_packet_action"
       },
       "enums" : {
           "mode" : {
               "TR_TCM" : "TWO_RATE_THREE_COLOR",
               "STORM_CONTROL" : "STORM_CONTROL",
               "SR_TCM" : "SINGLE_RATE_THREE_COLOR"
           },
           "green_packet_action" : {
               "DROP" : "DROP",
               "FORWARD" : "PERMIT",
               "COPY" : "COPY",
               "TRAP" : "TRAP"
           },
           "red_packet_action" : {
               "DROP" : "DROP",
               "FORWARD" : "PERMIT",
               "COPY" : "COPY",
               "TRAP" : "TRAP"
           }
       },
       "global_enums" : ["green_packet_action", "yellow_packet_action", "red_packet_action", "meter_type"],
       "enum_to_global" : {
         "green_packet_action" : "packet_action",
         "yellow_packet_action" : "packet_action",
         "red_packet_action" : "packet_action"
       }
   }
}
```

## Map generation ##
The map generation works by parsing the above JSON grammar.

The top-level "policer" corresponds to the SAI_OBJECT_TYPE_POLICER. The corresponding switch object is found as value for the "switch" key which in this case is "meter" (SWITCH_OBJECT_TYPE_METER).

The attributes are built as a chain of keys starting from the top-level "policer" key. So for the "mode" attribute, this becomes SAI_ + "policer" + _ATTR_ + "mode" which becomes SAI_POLICER_ATTR_MODE.

Similarly, enums are also chains of keys starting at top. SAI_ + "policer" + "mode" + "tr_tcm" which becomes SAI_POLICER_MODE_TR_TCM.

## Attribute conversion ##
 - SAI_POLICER_ATTR_MODE -> SWITCH_METER_ATTR_MODE
 - SAI_POLICER_ATTR_CBS -> SWITCH_METER_ATTR_CBS
 - SAI_POLICER_ATTR_GREEN_PACKET_ACTION -> SWITCH_METER_ATTR_GREEN_PACKET_ACTION

## ENUM conversion ##
For mode, the conversion is straight forward since the naming convention in SAI is predictable.
 - SAI_POLICER_MODE_TR_TCM -> SWITCH_METER_ATTR_MODE_TWO_RATE_THREE_COLOR
 - SAI_POLICER_MODE_STORM_CONTROL -> SWITCH_METER_ATTR_MODE_STORM_CONTROL

For green_packet_action though, this becomes SAI_POLICER_GREEN_PACKET_ACTION_DROP. This is not a SAI defined value. This is because the "green_packet_action" attribute uses a generic "sai_packet_action_t" type.

Hence, an additional level of conversion is required. The "enum_to_global" map helps with this secondary indirection.

The mapper looks at the global_enums and then subsitutes green_packet_action with packet_action from enum_to_global mapper.
 - SAI_PACKET_ACTION_DROP -> SWITCH_METER_ATTR_GREEN_PACKET_ACTION_DROP
 - SAI_PACKET_ACTION_FORWARD -> SWITCH_METER_ATTR_GREEN_PACKET_ACTION_PERMIT
