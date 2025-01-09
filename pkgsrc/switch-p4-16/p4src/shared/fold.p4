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



// --------------------------------------------------------------------------------------------------------
// Fold - Use below table to 2-fold, 4-fold or arbitary-fold switch pipeline
//
// Pipeline folding involves sending packets from
// ingress pipeline X -> egress pipeline Y -> ingress pipeline Y -> egress pipeline Z ..
//
// To achieve above forwarding behavior two things are needed
// 1. Ability to send Packet from Port x in ingress pipeline X to Port y in egress pipeline Y
// 2. Ability to loopback packet on egress pipeline Y port y to ingress pipeline Y port y
//
// The below table helps achievethe first of these two tasks.
// Specifically it allows for below behavior
// fold_4_pipe:
// ===========
// On a 4-pipe system 1:1 (same port on ingress/egress pipes) forwarding across pipe <N=X> -> Pipe <N=X+1>,
// where N > 0 and N < 4. The value of N wraps around so N goes from 0,1,2,3 -> 0
//
// fold_2_pipe:
// ===========
// On a 2-pipe system 1:1 forwarding across pipe <N=X> -> Pipe <N=X+1>,
// where N > 0 and N < 2. The value of N wraps around so N goes from 0,1 -> 0
//
// set_egress_port: Arbitary/User defined forwarding/folding
// ===============
// Match on any incoming ingress port and send packet to user defined egress Pipe/Port
//
// The Fold table should be added at the end of Switching Ingress Pipeline to override port forwarding
// ---------------------------------------------------------------------------------------------------------

#define FOLD                                                                                \
    action fold_4_pipe() {                                                                  \
        ig_intr_md_for_tm.ucast_egress_port = (ig_intr_md.ingress_port + 0x80);             \
    }                                                                                       \
                                                                                            \
    action fold_2_pipe() {                                                                  \
        ig_intr_md_for_tm.ucast_egress_port = (ig_intr_md.ingress_port ^ 0x80);             \
    }                                                                                       \
                                                                                            \
    action set_egress_port(switch_port_t dev_port) {                                        \
        ig_intr_md_for_tm.ucast_egress_port = dev_port;                                     \
    }                                                                                       \
                                                                                            \
    table fold {                                                                            \
        key = { ig_intr_md.ingress_port : exact; }                                          \
        actions = {                                                                         \
                    fold_2_pipe;                                                            \
                    fold_4_pipe;                                                            \
                    set_egress_port;                                                        \
                    }                                                                       \
        size = MIN_TABLE_SIZE;                                                              \
        default_action = fold_4_pipe;                                                       \
    }
