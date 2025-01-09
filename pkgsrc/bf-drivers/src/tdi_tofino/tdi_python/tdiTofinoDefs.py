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

from tdiDefs import *

"""
    These maps are based on the enums defined in tdi_tofino_defs.h/tna_defs.h
    If the enums there are changed, these maps must also be changed.
"""

# tdi_tofino_target_e
class TargetTypeTofino(TargetType):
    class TargetEnumTofino(Enum):
        TDI_TARGET_PIPE_ID = TargetEnum.TDI_TARGET_ARCH.value
        TDI_TARGET_DIRECTION = auto()
        TDI_TARGET_PARSER_ID = TargetEnum.TDI_TARGET_DEVICE.value

    target_type_dict = {
        TargetEnumTofino.TDI_TARGET_PIPE_ID.value   : "pipe_id",
        TargetEnumTofino.TDI_TARGET_DIRECTION.value : "direction",
        TargetEnumTofino.TDI_TARGET_PARSER_ID.value : "prsr_id"
    }
    target_type_rev_dict = {
        "pipe_id"   :   TargetEnumTofino.TDI_TARGET_PIPE_ID.value,
        "direction" :   TargetEnumTofino.TDI_TARGET_DIRECTION.value,
        "prsr_id"   :   TargetEnumTofino.TDI_TARGET_PARSER_ID.value
    }

    # merge with core map
    target_type_dict = {**TargetType.target_type_dict, **target_type_dict}
    target_type_rev_dict = {**TargetType.target_type_rev_dict, **target_type_rev_dict}


class FlagsTypeTofino(FlagsType):
    class FlagsEnumTofino(Enum):
        TDI_TOFINO_FLAGS_FROM_HW = FlagsEnum.TDI_FLAGS_DEVICE.value
        TDI_TOFINO_FLAGS_MOD_INC = auto()
        TDI_TOFINO_FLAGS_SKIP_TTL_RESET = auto()

    flags_dict = {
        FlagsEnumTofino.TDI_TOFINO_FLAGS_FROM_HW.value : "from_hw"
        # TODO: strings for other enums
    }
    flags_rev_dict = {
        "from_hw" : FlagsEnumTofino.TDI_TOFINO_FLAGS_FROM_HW.value
    }

    flags_dict = {**FlagsType.flags_dict, **flags_dict}
    flags_rev_dict = {**FlagsType.flags_rev_dict, **flags_rev_dict}

# tdi_tofino_attributes_type_e
class AttributesTypeTofino(AttributesType):
    class AttributesEnumTofino(Enum):
        TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE = AttributesEnum.TDI_ATTRIBUTES_TYPE_DEVICE.value
        TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK = auto()
        TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME = auto()
        TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ = auto()
        TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF = auto()
        TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS = auto()
        TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG = auto()
        TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK = auto()

    attributes_dict = {
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE.value: ["symmetric_mode_set", "symmetric_mode_get"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK.value: ["dynamic_key_mask_get", "dynamic_key_mask_set"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME.value: ["idle_table_set_poll", "idle_table_set_notify", "idle_table_get"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ.value: ["meter_byte_count_adjust_set", "meter_byte_count_adjust_get"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF.value: ["port_status_notif_cb_set"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS.value: ["port_stats_poll_intv_set", "port_stats_poll_intv_get"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_PRE_DEVICE_CONFIG.value: ["pre_device_config_set", "pre_device_config_get"],
        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK.value: ["selector_table_update_cb_set"]}
    attributes_rev_dict = {
                "EntryScope":           AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_ENTRY_SCOPE.value,
                "DynamicKeyMask":       AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_DYNAMIC_KEY_MASK.value,
                "IdleTimeout":          AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_IDLE_TABLE_RUNTIME.value,
                "MeterByteCountAdjust": AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_METER_BYTE_COUNT_ADJ.value,
                "port_status_notif_cb": AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STATUS_NOTIF.value,
                "poll_intvl_ms":        AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_PORT_STAT_POLL_INTVL_MS.value,
                "SelectorUpdateCb":     AttributesEnumTofino.TDI_TOFINO_ATTRIBUTES_TYPE_SELECTOR_UPDATE_CALLBACK.value
    }

# tdi_tofino_operations_type_e
class OperationsTypeTofino(OperationsType):
    class OperationsEnumTofino(Enum):
        TDI_TOFINO_OPERATIONS_TYPE_COUNTER_SYNC = OperationsEnum.TDI_OPERATIONS_TYPE_DEVICE.value
        TDI_TOFINO_OPERATIONS_TYPE_REGISTER_SYNC = auto()
        TDI_TOFINO_OPERATIONS_TYPE_HIT_STATUS_UPDATE = auto()
        TDI_TOFINO_OPERATIONS_TYPE_SYNC = auto()

    operations_dict = {
        OperationsEnumTofino.TDI_TOFINO_OPERATIONS_TYPE_COUNTER_SYNC.value        :   "operation_counter_sync",
        OperationsEnumTofino.TDI_TOFINO_OPERATIONS_TYPE_REGISTER_SYNC.value       :   "operation_register_sync",
        OperationsEnumTofino.TDI_TOFINO_OPERATIONS_TYPE_HIT_STATUS_UPDATE.value   :   "operation_hit_state_update"}

# tdi_tna_match_type_e
class KeyMatchTypeTofino(KeyMatchType):
    class KeyMatchEnumTofino(Enum):
        TDI_MATCH_TYPE_ATCAM = KeyMatchEnum.TDI_MATCH_TYPE_DEVICE.value

    key_match_type_dict = {
        KeyMatchEnumTofino.TDI_MATCH_TYPE_ATCAM.value: "ATCAM"}
    # merge with core map
    key_match_type_dict = {**KeyMatchType.key_match_type_dict, **key_match_type_dict}

# tdi_tofino_table_type_e
class TableTypeTofino(TableType):
    class TableTypeEnumTofino(Enum):
        TDI_TOFINO_TABLE_TYPE_MATCH_DIRECT = TableTypeEnum.TDI_TABLE_TYPE_DEVICE.value
        TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT = auto()
        TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT_SELECTOR = auto()
        TDI_TOFINO_TABLE_TYPE_ACTION_PROFILE = auto()
        TDI_TOFINO_TABLE_TYPE_SELECTOR = auto()
        TDI_TOFINO_TABLE_TYPE_COUNTER = auto()
        TDI_TOFINO_TABLE_TYPE_METER = auto()
        TDI_TOFINO_TABLE_TYPE_REGISTER = auto()
        TDI_TOFINO_TABLE_TYPE_LPF = auto()
        TDI_TOFINO_TABLE_TYPE_WRED = auto()
        TDI_TOFINO_TABLE_TYPE_PVS = auto()
        TDI_TOFINO_TABLE_TYPE_PORT_METADATA = auto()
        TDI_TOFINO_TABLE_TYPE_DYN_HASH_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_SNAPSHOT_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_SNAPSHOT_LIVENESS = auto()
        TDI_TOFINO_TABLE_TYPE_PORT_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_RECIRC_PORT_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_PORT_STAT = auto()
        TDI_TOFINO_TABLE_TYPE_PORT_HDL_INFO = auto()
        TDI_TOFINO_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO = auto()
        TDI_TOFINO_TABLE_TYPE_PORT_STR_INFO = auto()
        TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_PKTGEN_APP_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_PKTGEN_PKT_BUFF_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_MASK_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_DOWN_REPLAY_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_PRE_MGID = auto()
        TDI_TOFINO_TABLE_TYPE_PRE_NODE = auto()
        TDI_TOFINO_TABLE_TYPE_PRE_ECMP = auto()
        TDI_TOFINO_TABLE_TYPE_PRE_LAG = auto()
        TDI_TOFINO_TABLE_TYPE_PRE_PRUNE = auto()
        TDI_TOFINO_TABLE_TYPE_MIRROR_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PPG_OBSOLETE = auto()
        TDI_TOFINO_TABLE_TYPE_PRE_PORT = auto()
        TDI_TOFINO_TABLE_TYPE_DYN_HASH_ALGO = auto()
        TDI_TOFINO_TABLE_TYPE_TM_POOL_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_POOL_SKID = auto()
        TDI_TOFINO_TABLE_TYPE_DEV_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_POOL_APP = auto()
        TDI_TOFINO_TABLE_TYPE_TM_QUEUE_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_QUEUE_MAP = auto()
        TDI_TOFINO_TABLE_TYPE_TM_QUEUE_COLOR = auto()
        TDI_TOFINO_TABLE_TYPE_TM_QUEUE_BUFFER = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_GROUP_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_GROUP = auto()
        TDI_TOFINO_TABLE_TYPE_TM_POOL_COLOR = auto()
        TDI_TOFINO_TABLE_TYPE_SNAPSHOT_PHV = auto()
        TDI_TOFINO_TABLE_TYPE_SNAPSHOT_TRIG = auto()
        TDI_TOFINO_TABLE_TYPE_SNAPSHOT_DATA = auto()
        TDI_TOFINO_TABLE_TYPE_TM_POOL_APP_PFC = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_IG_PORT = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_EG_PORT = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_QUEUE = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_POOL = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_BUFFER = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_FLOWCONTROL = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PIPE = auto()
        TDI_TOFINO_TABLE_TYPE_DBG_CNT = auto()
        TDI_TOFINO_TABLE_TYPE_LOG_DBG_CNT = auto()
        TDI_TOFINO_TABLE_TYPE_TM_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PIPE_MULTICAST_FIFO = auto()
        TDI_TOFINO_TABLE_TYPE_TM_MIRROR_DPG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_DPG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PPG_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_REG_PARAM = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PORT_DPG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_MIRROR_PORT_DPG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PPG = auto()
        TDI_TOFINO_TABLE_TYPE_DYN_HASH_COMPUTE = auto()
        TDI_TOFINO_TABLE_TYPE_SELECTOR_GET_MEMBER = auto()
        TDI_TOFINO_TABLE_TYPE_TM_QUEUE_SCHED_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_QUEUE_SCHED_SHAPING = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_SCHED_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PORT_SCHED_SHAPING = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PIPE_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_PIPE_SCHED_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_L1_NODE_SCHED_CFG = auto()
        TDI_TOFINO_TABLE_TYPE_TM_L1_NODE_SCHED_SHAPING = auto()
        TDI_TOFINO_TABLE_TYPE_INVALID_TYPE = auto()

    table_type_dict = {
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_MATCH_DIRECT.value				:  "MATCH_DIRECT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT.value				:  "MATCH_INDIRECT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_MATCH_INDIRECT_SELECTOR.value		:  "MATCH_INDIRECT_SELECTOR",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_ACTION_PROFILE.value				:  "ACTION_PROFILE",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SELECTOR.value					:  "SELECTOR",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_COUNTER.value						:  "COUNTER",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_METER.value						:  "METER",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_REGISTER.value					:  "REGISTER",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_LPF.value							:  "LPF",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_WRED.value						:  "WRED",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PVS.value							:  "PVS",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PORT_METADATA.value				:  "PORT_METADATA",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_DYN_HASH_CFG.value				:  "DYN_HASH_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SNAPSHOT_CFG.value				:  "SNAPSHOT_CFG",          # /** Snapshot. */
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SNAPSHOT_LIVENESS.value			:  "SNAPSHOT_LIVENESS",          # /** Snapshot. */
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PORT_CFG.value					:  "PORT_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_RECIRC_PORT_CFG.value				:  "RECIRC_PORT_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PORT_STAT.value					:  "PORT_STAT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PORT_HDL_INFO.value				:  "PORT_HDL_INFO",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PORT_FRONT_PANEL_IDX_INFO.value	:  "PORT_FRONT_PANEL_IDX_INFO",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PORT_STR_INFO.value				:  "PORT_STR_INFO",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_CFG.value				:  "PKTGEN_PORT_CFG",     # /** Pktgen Port Configuration table */
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PKTGEN_APP_CFG.value				:  "PKTGEN_APP_CFG",      # /** Pktgen Application Configuration table */
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PKTGEN_PKT_BUFF_CFG.value			:  "PKTGEN_PKT_BUFF_CFG", # /** Pktgen Packet Buffer Configuration table */
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_MASK_CFG.value		:  "PKTGEN_PORT_MASK_CFG", # /** Pktgen Port Mask Configuration table */
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PKTGEN_PORT_DOWN_REPLAY_CFG.value	:  "PKTGEN_PORT_DOWN_REPLAY_CFG", # /** Pktgen Port Down Replay Configuration table*/
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PRE_MGID.value					:  "PRE_MGID",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PRE_NODE.value					:  "PRE_NODE",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PRE_ECMP.value					:  "PRE_ECMP",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PRE_LAG.value						:  "PRE_LAG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PRE_PRUNE.value					:  "PRE_PRUNE",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_MIRROR_CFG.value					:  "MIRROR_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PPG_OBSOLETE.value				:  "TM_PPG_OBSOLETE", # retired
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_PRE_PORT.value					:  "PRE_PORT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_DYN_HASH_ALGO.value				:  "DYN_HASH_ALGO",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_POOL_CFG.value					:  "TM_POOL_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_POOL_SKID.value				:  "TM_POOL_SKID",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_DEV_CFG.value						:  "DEV_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_POOL_APP.value					:  "TM_POOL_APP",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_QUEUE_CFG.value				:  "TM_QUEUE_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_QUEUE_MAP.value				:  "TM_QUEUE_MAP",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_QUEUE_COLOR.value				:  "TM_QUEUE_COLOR",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_QUEUE_BUFFER.value				:  "TM_QUEUE_BUFFER",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_GROUP_CFG.value			:  "TM_PORT_GROUP_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_GROUP.value				:  "TM_PORT_GROUP",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_POOL_COLOR.value				:  "TM_POOL_COLOR",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SNAPSHOT_PHV.value				:  "SNAPSHOT_PHV",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SNAPSHOT_TRIG.value				:  "SNAPSHOT_TRIG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SNAPSHOT_DATA.value				:  "SNAPSHOT_DATA",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_POOL_APP_PFC.value				:  "TM_POOL_APP_PFC",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_IG_PORT.value			:  "TM_COUNTER_IG_PORT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_EG_PORT.value			:  "TM_COUNTER_EG_PORT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_QUEUE.value			:  "TM_COUNTER_QUEUE",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_POOL.value				:  "TM_COUNTER_POOL",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_CFG.value					:  "TM_PORT_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_BUFFER.value				:  "TM_PORT_BUFFER",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_FLOWCONTROL.value			:  "TM_PORT_FLOWCONTROL",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PIPE.value				:  "TM_COUNTER_PIPE",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_DBG_CNT.value						:  "DBG_CNT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_LOG_DBG_CNT.value					:  "LOG_DBG_CNT",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_CFG.value						:  "TM_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PIPE_MULTICAST_FIFO.value		:  "TM_PIPE_MULTICAST_FIFO",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_MIRROR_DPG.value				:  "TM_MIRROR_DPG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_DPG.value					:  "TM_PORT_DPG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PPG_CFG.value					:  "TM_PPG_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_REG_PARAM.value					:  "REG_PARAM",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PORT_DPG.value			:  "TM_COUNTER_PORT_DPG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_MIRROR_PORT_DPG.value	:  "TM_COUNTER_MIRROR_PORT_DPG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_COUNTER_PPG.value				:  "TM_COUNTER_PPG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_DYN_HASH_COMPUTE.value			:  "DYN_HASH_COMPUTE",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_SELECTOR_GET_MEMBER.value			:  "SELECTOR_GET_MEMBER",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_QUEUE_SCHED_CFG.value			:  "TM_QUEUE_SCHED_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_QUEUE_SCHED_SHAPING.value		:  "TM_QUEUE_SCHED_SHAPING",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_SCHED_CFG.value			:  "TM_PORT_SCHED_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PORT_SCHED_SHAPING.value		:  "TM_PORT_SCHED_SHAPING",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PIPE_CFG.value					:  "TM_PIPE_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_PIPE_SCHED_CFG.value			:  "TM_PIPE_SCHED_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_L1_NODE_SCHED_CFG.value		:  "TM_L1_NODE_SCHED_CFG",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_TM_L1_NODE_SCHED_SHAPING.value	:  "TM_L1_NODE_SCHED_SHAPING",
        TableTypeEnumTofino.TDI_TOFINO_TABLE_TYPE_INVALID_TYPE.value				:  "INVLD"
    }

    # merge with core map
    table_type_dict = {**TableType.table_type_dict, **table_type_dict}
