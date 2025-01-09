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

"C000    +ch_num * 0x400 Number of frames transmitted with out Error.",
    "C008    +ch_num * 0x400 Number of frames transmitted overall (Good/Bad "
    "Frames)",
    "C010    +ch_num * 0x400 Number of frames transmitted with any error "
    "(CRC).",
    "C018    +ch_num * 0x400 Number of bytes transmitted in all the frames "
    "(Error free frames)",
    "C020    +ch_num * 0x400 Number of bytes transmitted in all frames "
    "(good/error frames)",
    "C028    +ch_num * 0x400 Number of frames transmitted with Unicast Address",
    "C030    +ch_num * 0x400 Number of frames transmitted with Multicast "
    "Address",
    "C038    +ch_num * 0x400 Number of frames transmitted with Broadcast "
    "Address",
    "C040    +ch_num * 0x400 Number of frames transmitted that are PAUSE type "
    "frames",
    "C048    +ch_num * 0x400 Number of frames transmitted that are Priority "
    "PAUSE type frames",
    "C050    +ch_num * 0x400 Number of frames transmitted that are VLAN type",
    "C058    +ch_num * 0x400 Number of frames transmitted whose size is less "
    "than 64-bytes.",
    "C060    +ch_num * 0x400 Number of frames transmitted whose size is equal "
    "to 64-bytes.",
    "C068    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 65 to 127 bytes.",
    "C070    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 128 to 255 bytes.",
    "C078    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 256 to 511 bytes.",
    "C080    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 512 to 1023 bytes.",
    "C088    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 1024 to 1518 bytes.",
    "C090    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 1519 to 2047 bytes.",
    "C098    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 2048 to 4095 bytes.",
    "C0A0    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 4096 to 8191 bytes.",
    "C0A8    +ch_num * 0x400 Number of frames transmitted whose size is within "
    "the range of 8192 to 9215 bytes.",
    "C0B0    +ch_num * 0x400 Number of frames transmitted whose size is equal "
    "to or greater than 9216.",
    "C0B8    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#0 is set.",
    "C0C0    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#1 is set.",
    "C0C8    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#2 is set.",
    "C0D0    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#3 is set.",
    "C0D8    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#4 is set.",
    "C0E0    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#5 is set.",
    "C0E8    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#6 is set.",
    "C0F0    +ch_num * 0x400 Number of frames transmitted of Priority PAUSE "
    "type and whose Priority#7 is set.",
    "C0F8    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#0.",
    "C100    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#1.",
    "C108    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#2.",
    "C110    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#3.",
    "C118    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#4.",
    "C120    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#5.",
    "C128    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#6.",
    "C130    +ch_num * 0x400 Duration of XOFF time (in us) for Transmit "
    "Priority#7.",
    "C138    +ch_num * 0x400 Number of frames that were drained",
    "C140    +ch_num * 0x400 Number of frames transmitted that were jabberred",
    "C148    +ch_num * 0x400 Number of frames transmitted that were padded",
    "C150    +ch_num * 0x400 Number of frames that were truncated",
    "C158    +ch_num * 0x400 Number of frames received with out Error.",
    "C160    +ch_num * 0x400 Number of bytes received in all the frames (Error "
    "free frames)",
    "C168    +ch_num * 0x400 Number of frames received overall (Good/Bad "
    "Frames)",
    "C170    +ch_num * 0x400 Number of bytes received in all frames "
    "(good/error frames)",
    "C178    +ch_num * 0x400 Number of frames received with CRC Error.",
    "C180    +ch_num * 0x400 Number of frames received with any error (CRC, "
    "Length, Alignment, Runt, Jabber etc)",
    "C188    +ch_num * 0x400 Number of frames received with Unicast Address",
    "C190    +ch_num * 0x400 Number of frames received with Multicast Address",
    "C198    +ch_num * 0x400 Number of frames received with Broadcast Address",
    "C1A0    +ch_num * 0x400 Number of frames received that are PAUSE type "
    "frames",
    "C1A8    +ch_num * 0x400 Number of frames received with Length Error",
    "C1B0    +ch_num * 0x400 Reserved",
    "C1B8    +ch_num * 0x400 Number of Frames received whose size is greater "
    "than programmed MaxFrameSize and are error free",
    "C1C0    +ch_num * 0x400 Number of frames received which are runt (less "
    "than MinFrameSize) and have CRC error.",
    "C1C8    +ch_num * 0x400 Number of frames received which exceed the "
    "programmed Jabber size and have CRC Error.",
    "C1D0    +ch_num * 0x400 Number of frames received that are Priority PAUSE "
    "type frames.",
    "C1D8    +ch_num * 0x400 Number of frames received whose CRC was stomped.",
    "C1E0    +ch_num * 0x400 Number of Frames received whose size exceed the "
    "MaxFrameSize but are less than Jabber size.",
    "C1E8    +ch_num * 0x400 Number of frames received which have VLAN (good "
    "frames)",
    "C1F0    +ch_num * 0x400 Number of frames that were dropped from APP_FIFO "
    "because of Buffer Full condition",
    "C1F8    +ch_num * 0x400 Number of frames received whose size is less than "
    "64-bytes.",
    "C200    +ch_num * 0x400 Number of frames received whose size is equal to "
    "64-bytes.",
    "C208    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 65 to 127 bytes.",
    "C210    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 128 to 255 bytes.",
    "C218    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 256 to 511 bytes.",
    "C220    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 512 to 1023 bytes.",
    "C228    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 1024 to 1518 bytes.",
    "C230    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 1519 to 2047 bytes.",
    "C238    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 2048 to 4095 bytes.",
    "C240    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 4096 to 8191 bytes.",
    "C248    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 8192 to 9215 bytes.",
    "C250    +ch_num * 0x400 Number of frames received whose size is equal to "
    "or greater than 9216.",
    "C258    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#0 is set.",
    "C260    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#1 is set.",
    "C268    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#2 is set.",
    "C270    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#3 is set.",
    "C278    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#4 is set.",
    "C280    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#5 is set.",
    "C288    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#6 is set.",
    "C290    +ch_num * 0x400 Number of frames received of Priority PAUSE type "
    "and whose Priority#7 is set.",
    "C298    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#0.",
    "C2A0    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#1.",
    "C2A8    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#2.",
    "C2B0    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#3.",
    "C2B8    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#4.",
    "C2C0    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#5.",
    "C2C8    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#6.",
    "C2D0    +ch_num * 0x400 Duration of XOFF time (in us) for Receive "
    "Priority#7.",
    "C2D8    +ch_num * 0x400 Duration of XOFF time (in us) for Receive PAUSE "
    "(Standard)",
    "C2E0    +ch_num * 0x400 Number of frames received which are truncated in "
    "APP_FIFO.",
    "C2E8    +ch_num * 0x400 Reserved",
    "C2F0    +ch_num * 0x400 Number of Frames received with Invalid Preamble.",
    "C2F8    +ch_num * 0x400 Number of frames received whose size is within "
    "the range of 64 to 1518B and have CRC Error",
