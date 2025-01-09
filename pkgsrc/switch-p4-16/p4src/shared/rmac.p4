/*******************************************************************************
 *  Copyright (C) 2024 Intel Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions
 *  and limitations under the License.
 *
 *
 *  SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/



control IngressRmac(inout switch_header_t hdr,
                    inout switch_local_metadata_t local_md)(
                    switch_uint32_t port_vlan_table_size,
                    switch_uint32_t vlan_table_size=4096) {
    //
    // **************** Router MAC Check ************************
    //
    @name(".rmac_miss")
    action rmac_miss() {
//        local_md.flags.rmac_hit = false;
    }
    @name(".rmac_hit")
    action rmac_hit() {
        local_md.flags.rmac_hit = true;
    }

    @name(".pv_rmac")
    table pv_rmac {
        key = {
            local_md.ingress_port_lag_index : ternary;
            hdr.vlan_tag[0].isValid() : ternary;
            hdr.vlan_tag[0].vid : ternary;
            hdr.ethernet.dst_addr : ternary;
        }

        actions = {
            rmac_miss;
            rmac_hit;
        }

        const default_action = rmac_miss;
        size = port_vlan_table_size;
    }

    @name(".vlan_rmac")
    table vlan_rmac {
        key = {
            hdr.vlan_tag[0].vid : exact;
            hdr.ethernet.dst_addr : exact;
        }

        actions = {
            @defaultonly rmac_miss;
            rmac_hit;
        }

        const default_action = rmac_miss;
        size = vlan_table_size;
    }

    apply {
        switch (pv_rmac.apply().action_run) {
            rmac_miss : {
                if (hdr.vlan_tag[0].isValid())
                    vlan_rmac.apply();
            }
        }
    }
}
