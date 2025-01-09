# Copyright (c) 2022 Intel Corporation.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0.
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
from bf_pktpy.library.specs.packet import Packet
from bf_pktpy.library.fields import ByteField, ShortField


class MirrorPreDeparser(Packet):
    name = "MirrorIntMdHeader"
    fields_desc = [
        ByteField("pkt_type", 0x01),
        ByteField("do_egr_mirroring", 0x01),
        ShortField("sid", 0x00),
    ]

    def _combine(self, body_copy):
        if body_copy.name == "Ether":
            self._body = body_copy
            return self

        raise ValueError("Unsupported binding")
