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

import binascii
import ptf
from ptf.base_tests import BaseTest
import ptf.testutils as testutils

import time
import grpc
import bfrt_grpc.bfruntime_pb2_grpc as bfruntime_pb2_grpc
import bfrt_grpc.bfruntime_pb2 as bfruntime_pb2

import six
if six.PY2:
    import Queue
else:
    import queue as Queue
import logging
import threading
import json
import sys
import random
import math
import socket

import google.rpc.status_pb2 as status_pb2
import google.rpc.code_pb2 as code_pb2

from collections import namedtuple

logger = logging.getLogger('BfRuntimeBaseTest')
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.DEBUG)

class TableInfo:
    def __init__(self, table_id):
        self.action_dict = {}
        self.key_dict = {}
        self.data_dict = {}
        self.id = table_id
        self.size = 0

class LearnInfo:
    def __init__(self, learn_id):
        self.data_dict = {}
        self.id = learn_id
class DataInfo:
    def __init__(self, data_id):
        self.id = data_id
class KeyInfo:
    def __init__(self, key_id):
        self.id = key_id
class ActionInfo:
    def __init__(self, action_id):
        self.data_dict = {}
        self.id = action_id

class BfRtInfo:
    def __init__(self, json_data):
        self.table_dict = {}
        self.learn_dict = {}

        # parse tables
        bfrtinfo_json = json.loads(six.ensure_str(json_data))

        self._insert_objs_in_dict(
                self.table_dict,
                bfrtinfo_json["tables"],
                True)
        self._insert_objs_in_dict(
                self.learn_dict,
                bfrtinfo_json["learn_filters"],
                False)

    def _insert_objs_in_dict(self, main_obj_dict, json_list, is_table=True):
        """@brief Add objects to the main dictionary. Same object
            can be referenced by many names depending upon uniqueness
        """
        names_to_remove = set()
        for idx, obj_info in enumerate(json_list):
            obj_name = obj_info["name"]
            # Make all possible names for the object and try to insert them
            # in the dictionary
            obj = None
            if is_table:
                obj = self.parse_table(obj_info)
            else:
                obj = self.parse_learn(obj_info)
            possible_name_list = BfRtInfo._generate_unique_names(obj_name)
            for prospective_name in possible_name_list:
                if prospective_name in main_obj_dict:
                    names_to_remove.add(prospective_name)
                else:
                    main_obj_dict[prospective_name] = obj
        for name in names_to_remove:
            main_obj_dict.pop(name, None)

    @staticmethod
    def _generate_unique_names(table_name):
        tokens = table_name.split(".")
        name_list = set()
        last_token = ""
        for token in reversed(tokens):
            if not last_token:
                name_list.add(token)
                last_token = token
            else:
                name_list.add(token+"."+last_token)
                last_token = token+"."+last_token
        return name_list

    def __str__(self):
        msg = "TABLES->\n"
        for table_name, table in six.iteritems(self.table_dict):
            msg += table_name + ":" + str(table.id) + "\n"
            msg += "\tKEYS->\n"
            for key_name, key in six.iteritems(table.key_dict):
                msg += "\t" + key_name + ":" + str(key.id) + "\n"
            msg += "\tACTIONS->\n"
            for action_name, action in six.iteritems(table.action_dict):
                msg += "\t" + action_name + ":" + str(action.id) + "\n"
                msg += "\t\tDATA->\n"
                for data_name, data in six.iteritems(action.data_dict):
                    msg += "\t\t" + data_name + ":" + str(data.id) + "\n"
            msg += "\tDATA->\n"
            for data_name, data in six.iteritems(table.data_dict):
                msg += "\t" + data_name + ":" + str(data.id) + "\n"
        return msg

    def parse_table(self, table_json):
        table_obj = TableInfo(table_json["id"])
        for key_json in table_json["key"]:
            table_obj.key_dict[key_json["name"]] = self.parse_key(key_json)
        if ("action_specs" in table_json):
            for action_json in table_json["action_specs"]:
                table_obj.action_dict[action_json["name"]] = self.parse_action(action_json)

        for data_json in table_json["data"]:
            self.parse_data_helper(table_obj.data_dict, data_json)
        table_obj.size = table_json["size"]
        return table_obj

    def parse_key(self, key_json):
        key_obj = KeyInfo(key_json["id"])
        return key_obj

    def parse_action(self, action_json):
        action_obj = ActionInfo(action_json["id"])
        for data_json in action_json["data"]:
            self.parse_data_helper(action_obj.data_dict, data_json)
        return action_obj

    def parse_data_helper(self, input_dict, data_json):
        if "oneof" in data_json:
            input_dict[data_json["oneof"][0]["name"]] = self.parse_data(data_json["oneof"][0])
            input_dict[data_json["oneof"][1]["name"]] = self.parse_data(data_json["oneof"][1])
        elif "singleton" in data_json:
            input_dict[data_json["singleton"]["name"]] = self.parse_data(data_json["singleton"])
            if "container" in data_json["singleton"]:
                # Containers contains a set of fields that can be repeated.
                # A container can contain another container.
                for container_data_json in data_json["singleton"]["container"]:
                    self.parse_data_helper(input_dict, container_data_json)
        else:
            input_dict[data_json["name"]] = self.parse_data(data_json)

    def parse_data(self, data_json):
        data_obj = DataInfo(data_json["id"])
        return data_obj

    def parse_learn(self, learn_json):
        learn_obj = LearnInfo(learn_json["id"])
        for data_json in learn_json["fields"]:
            self.parse_data_helper(learn_obj.data_dict, data_json)
        return learn_obj

def parseGrpcErrorBinaryDetails(grpc_error):
    if grpc_error.code() != grpc.StatusCode.UNKNOWN:
        return None

    error = None
    # The gRPC Python package does not have a convenient way to access the
    # binary details for the error: they are treated as trailing metadata.
    for meta in grpc_error.trailing_metadata():
        if meta[0] == "grpc-status-details-bin":
            error = status_pb2.Status()
            error.ParseFromString(meta[1])
            break
    if error is None:  # no binary details field
        return None
    if len(error.details) == 0:
        # binary details field has empty Any details repeated field
        return None

    indexed_p4_errors = []
    for idx, one_error_any in enumerate(error.details):
        p4_error = bfruntime_pb2.Error()
        if not one_error_any.Unpack(p4_error):
            return None
        if p4_error.canonical_code == code_pb2.OK:
            continue
        indexed_p4_errors += [(idx, p4_error)]
    return indexed_p4_errors

def printGrpcError(grpc_error):
    status_code = grpc_error.code()
    logger.error("gRPC Error %s %s",
            grpc_error.details(),
            status_code.name)

    if status_code != grpc.StatusCode.UNKNOWN:
        return
    bfrt_errors = parseGrpcErrorBinaryDetails(grpc_error)
    if bfrt_errors is None:
        return
    logger.error("Errors in batch:")
    for idx, bfrt_error in bfrt_errors:
        code_name = code_pb2._CODE.values_by_number[
            bfrt_error.canonical_code].name
        logger.error("\t* At index %d %s %s\n",
                idx, code_name, bfrt_error.message)
    return bfrt_errors

# This is common to all tests. setUp() is invoked at the begining of the test
# and tearDown() is called at the end, regardless of whether the test is passed
# or failed/errored.
class BfRuntimeTest(BaseTest):
    def __init__(self):
        BaseTest.__init__(self)
        self.device_id = 0
        self.client_id = 0
        self._swports = []
        self.bfrt_info = None
        self.p4_name = ""


    def tearDown(self):
        self.tear_down_stream()
        BaseTest.tearDown(self)

    def setUp(self, client_id = None, p4_name = None):
        BaseTest.setUp(self)

        # Setting up PTF dataplane
        self.dataplane = ptf.dataplane_instance
        self.dataplane.flush()

        grpc_addr = testutils.test_param_get("grpc_server")
        if grpc_addr is None or grpc_addr == 'localhost':
            grpc_addr = 'localhost:50052'
        else:
            grpc_addr = grpc_addr + ":50052"

        if client_id != None:
            self.client_id = client_id
        else:
            self.client_id = 0

        if p4_name != None:
            self.p4_name = p4_name
        else:
            self.p4_name = ""

        gigabyte = 1024 ** 3
        self.channel = grpc.insecure_channel(grpc_addr, options=[
            ('grpc.max_send_message_length', gigabyte), (
                'grpc.max_receive_message_length', gigabyte),
                ('grpc.max_metadata_size', gigabyte)])

        self.stub = bfruntime_pb2_grpc.BfRuntimeStub(self.channel)

        self.set_up_stream()

        # Subscribe to receive notifications
        num_tries = 5
        cur_tries = 0
        success = False
        while(cur_tries < num_tries and not success):
            self.subscribe()
            logger.info("Subscribe attempt #%d", cur_tries+1)
            # Wait for 5 seconds max for each attempt
            success = self.is_subscribe_successful(5)
            cur_tries += 1
        # Set forwarding pipeline config (For the time being we are just
        # associating a client with a p4). Currently the grpc server supports
        # only one client to be in-charge of one p4.
        if p4_name and p4_name != "":
            self.bindPipelineConfig()


    def bindPipelineConfig(self):
        req = bfruntime_pb2.SetForwardingPipelineConfigRequest()
        req.client_id = self.client_id;
        req.action = bfruntime_pb2.SetForwardingPipelineConfigRequest.BIND;
        config = req.config.add()
        config.p4_name = self.p4_name
        logger.info("Binding with p4_name " + self.p4_name)
        try:
            self.stub.SetForwardingPipelineConfig(req)
        except grpc.RpcError as e:
            if e.code() != grpc.StatusCode.UNKNOWN:
                raise e
        logger.info("Binding with p4_name %s successful!!", self.p4_name)

    InputConfig = namedtuple('Input_config', 'profile_name context_file binary_file pipe_scope')
    InputConfig.__new__.__defaults__ = ("", "", "", [])

    def addConfigToSetForwardRequest(self, req, p4_name, bfruntime_info,
            input_profiles):
        def read_file(file_name):
            data = ""
            with open(file_name, 'r') as myfile:
                data=myfile.read()
            return data
        config = req.config.add()
        config.p4_name = p4_name
        config.bfruntime_info = read_file(bfruntime_info)
        for input_profile in input_profiles:
            profile = config.profiles.add()
            profile.profile_name = input_profile.profile_name
            profile.context = read_file(input_profile.context_file)
            profile.binary = read_file(input_profile.binary_file)
            profile.pipe_scope.extend(input_profile.pipe_scope)


    def write(self, req):
        req.client_id = self.client_id
        try:
            self.stub.Write(req)
        except grpc.RpcError as e:
            printGrpcError(e)
            raise e

    def read(self, req):
        try:
            return self.stub.Read(req)
        except grpc.RpcError as e:
            printGrpcError(e)
            raise e

    # Most of these helper functions are borrowed from p4runtime_base_test
    # See https://github.com/barefootnetworks/bf-p4c-compilers/blob/master/p4-tests/base_test.py
    def set_up_stream(self):
        self._stream_out_q = Queue.Queue()
        self._stream_in_q = Queue.Queue()
        self._exception_q = Queue.Queue()
        def stream_iterator():
            while True:
                p = self._stream_out_q.get()
                if p is None:
                    break
                yield p

        def stream_recv(stream):
            try:
                for p in stream:
                    self._stream_in_q.put(p)
            except grpc.RpcError as e:
                self._exception_q.put(e)

        self.stream = self.stub.StreamChannel(stream_iterator())
        self._stream_recv_thread = threading.Thread(
            target=stream_recv, args=(self.stream,))
        self._stream_recv_thread.start()

    def subscribe(self):
        req = bfruntime_pb2.StreamMessageRequest()
        req.client_id = self.client_id
        req.subscribe.device_id = self.device_id
        req.subscribe.notifications.enable_learn_notifications = True
        req.subscribe.notifications.enable_idletimeout_notifications = True
        req.subscribe.notifications.enable_port_status_change_notifications = True
        self._stream_out_q.put(req)

    def get_packet_in(self, timeout=1):
        pass

    def is_subscribe_successful(self, timeout=1):
        msg = self.get_stream_message("subscribe", timeout)
        if msg is None:
            logger.info("Subscribe timeout exceeded %ds", timeout)
            return False
        else:
            logger.info("Subscribe response received %d", msg.subscribe.status.code)
            if (msg.subscribe.status.code != code_pb2.OK):
                logger.info("Subscribe failed")
                return False
        return True

    def is_set_fwd_action_done(self, value_to_check_for, timeout=1):
        msg = self.get_stream_message("set_forwarding_pipeline_config_response", timeout)
        if msg is None:
            logger.info("commit notification expectation exceeded %ds", timeout)
            return False
        else:
            if (msg.set_forwarding_pipeline_config_response.
                    set_forwarding_pipeline_config_response_type == value_to_check_for ==
                    bfruntime_pb2.SetForwardingPipelineConfigResponseType.Value("WARM_INIT_STARTED")):
                logger.info("WARM_INIT_STARTED received")
                return True
            elif (msg.set_forwarding_pipeline_config_response.
                    set_forwarding_pipeline_config_response_type == value_to_check_for ==
                    bfruntime_pb2.SetForwardingPipelineConfigResponseType.Value("WARM_INIT_FINISHED")):
                logger.info("WARM_INIT_FINISHED received")
                return True
        return False

    def get_digest(self, timeout=1):
        msg = self.get_stream_message("digest", timeout)
        if msg is None:
            self.fail("Digest list not received.")
        else:
            return msg.digest

    def get_bfrt_info(self, p4_name="", timeout=1):
        # send a request
        req = bfruntime_pb2.GetForwardingPipelineConfigRequest()
        req.device_id = self.device_id
        req.client_id = self.client_id
        msg = self.stub.GetForwardingPipelineConfig(req)

        # get the reply
        if msg is None:
            self.fail("BF_RT_INFO not received")
        else:
            if (p4_name == ""):
                return msg.config[0].bfruntime_info
            for config in msg.config:
                logger.info("Received %s on GetForwarding", config.p4_name)
            for config in msg.config:
                if (p4_name == config.p4_name):
                    return msg.config[0].bfruntime_info
            self.fail("BF_RT_INFO not received")

    def parse_bfrt_info(self, data):
        return BfRtInfo(data)

    def set_bfrt_info(self, bfrt_info):
        self.bfrt_info = bfrt_info

    def get_idletime_notification(self, timeout=1):
        msg = self.get_stream_message("idle_timeout_notification", timeout)
        if msg is not None:
            return msg.idle_timeout_notification
        return None

    def get_portstatus_notification(self, timeout=1):
        msg = self.get_stream_message("port_status_change_notification", timeout)
        if msg is None:
            self.fail("port_status_change_notification not received.")
        else:
            return msg.port_status_change_notification

    def get_stream_message(self, type_, timeout=1):
        start = time.time()
        try:
            while True:
                remaining = timeout - (time.time() - start)
                if remaining < 0:
                    break
                msg = self._stream_in_q.get(timeout=remaining)
                if not msg.HasField(type_):
                    # Put the msg back in for someone else to read
                    # TODO make separate queues for each msg type
                    self._stream_in_q.put(msg)
                    continue
                return msg
        except:  # timeout expired
            pass
        return None

    def tear_down_stream(self):
        self._stream_out_q.put(None)
        self._stream_recv_thread.join()

    def get_table_local(self, table_name):
        for table_name_, table_ in six.iteritems(self.bfrt_info.table_dict):
            if (table_name_ == table_name):
                return table_.id

    def get_table(self, name):
        if self.bfrt_info:
            return self.get_table_local(name)
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        req.target.device_id = self.device_id

        object_id = req.entities.add().object_id
        object_id.table_object.table_name = name

        for rep in self.stub.Read(req):
            return rep.entities[0].object_id.id

    def get_action_local(self, table_name, action_name):
        table_obj = self.bfrt_info.table_dict[table_name]
        for action_name_, action_ in six.iteritems(table_obj.action_dict):
            if (action_name_ == action_name):
                return action_.id

    def get_action(self, table_name, action_name):
        """ Get action id for a given table and action. """
        if self.bfrt_info:
            return self.get_action_local(table_name, action_name)
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        req.target.device_id = self.device_id

        object_id = req.entities.add().object_id
        object_id.table_object.table_name = table_name
        object_id.table_object.action_name.action = action_name

        for rep in self.stub.Read(req):
            return rep.entities[0].object_id.id

    def get_data_field_local(self, table_name, action_name, field_name):
        table_obj = self.bfrt_info.table_dict[table_name]
        if action_name is not None:
            for action_name_, action_ in six.iteritems(table_obj.action_dict):
                if action_name_ == action_name:
                    for field_name_, data_ in six.iteritems(action_.data_dict):
                        if field_name_ == field_name:
                            return data_.id
        for field_name_, data_ in six.iteritems(table_obj.data_dict):
            if field_name_ == field_name:
                return data_.id
        return 0

    def get_data_field(self, table_name, action_name, field_name):
        """ Get data field id for a given table, action and field. """
        if self.bfrt_info:
            return self.get_data_field_local(table_name, action_name, field_name)
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        req.target.device_id = self.device_id

        object_id = req.entities.add().object_id
        object_id.table_object.table_name = table_name
        if action_name is not None:
            object_id.table_object.data_field_name.action = action_name
        object_id.table_object.data_field_name.field = field_name
        for rep in self.stub.Read(req):
            return rep.entities[0].object_id.id

    def get_learn_data_field_local(self, learn_name, field_name):
        learn_obj = self.bfrt_info.learn_dict[learn_name]
        for field_name_, data_ in six.iteritems(learn_obj.data_dict):
            if field_name_ == field_name:
                return data_.id
        return 0

    def get_learn_data_field(self, learn_name, field_name):
        """ Get data field id for a given table, action and field. """
        if self.bfrt_info:
            return self.get_learn_data_field_local(learn_name, field_name)
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        req.target.device_id = self.device_id

        object_id = req.entities.add().object_id
        object_id.learn_object.learn_name = learn_name
        object_id.learn_object.data_field_name.field = field_name
        for rep in self.stub.Read(req):
            return rep.entities[0].object_id.id

    def get_key_local(self, table_name, field_name):
        table_obj = self.bfrt_info.table_dict[table_name]
        for field_name_, key_ in six.iteritems(table_obj.key_dict):
            if field_name_ == field_name:
                return key_.id
        return 0

    def get_key_field(self, table_name, field_name):
        """ Get key field id for a given table and field. """
        if self.bfrt_info:
            return self.get_key_local(table_name, field_name)
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        req.target.device_id = self.device_id

        object_id = req.entities.add().object_id
        object_id.table_object.table_name = table_name
        object_id.table_object.key_field_name.field = field_name
        for rep in self.stub.Read(req):
            return rep.entities[0].object_id.id

    def get_key_name_local(self, table_name, field_id):
        table_obj = self.bfrt_info.table_dict[table_name]
        for field_name_, key_ in six.iteritems(table_obj.key_dict):
            if key_.id == field_id:
                return field_name_
        return ''

    def get_key_name(self, table_name, field_id):
        """ Get key field name for a given table and field id. """
        if self.bfrt_info:
            return self.get_key_name_local(table_name, field_id)
        assert False

    def swports(self, idx):
        if idx >= len(self._swports):
            self.fail("Index {} is out-of-bound of port map".format(idx))
            return None
        return self._swports[idx]

    # Helper functions to make writing BfRuntime PTF tests easier.
    DataField = namedtuple('data_field', 'name stream float_val str_val int_arr_val bool_arr_val bool_val')
    DataField.__new__.__defaults__ = (None, None, None, None, None, None, None)

    KeyField = namedtuple('key_field', 'name value mask prefix_len low high')
    KeyField.__new__.__defaults__ = (None, None, None, None, None, None)

    Target =  namedtuple('target', 'device_id pipe_id direction prsr_id')
    Target.__new__.__defaults__ = (0, 0xffff, 0xff, 0xff)

    IpRandom = namedtuple('ip_random', 'ip prefix_len mask')
    MacRandom = namedtuple('mac_random', 'mac mask')

    def parseTableUsage(self, response, table_id_dict):
        '''
        table_id_dict is a dictionary of table_ids
        '''
        for rep in response:
            for entity in rep.entities:
                table_usage_response = entity.table_usage
                for table_id_query, usage_val in six.iteritems(table_id_dict):
                    if table_usage_response.table_id == table_id_query:
                        table_id_dict[table_id_query] = table_usage_response.usage
                # Yielding here allows to iterate over more entities
                yield

    def parseKey(self, key, key_dict):
        for key_field in key.fields:
            key_dict[key_field.field_id] = {}
            if key_field.HasField("exact"):
                key_dict[key_field.field_id]["value"] = key_field.exact.value
            elif key_field.HasField("ternary"):
                key_dict[key_field.field_id]["value"] = key_field.ternary.value
                key_dict[key_field.field_id]["mask"] = key_field.ternary.mask
            elif key_field.HasField("lpm"):
                key_dict[key_field.field_id]["value"] = key_field.lpm.value
                key_dict[key_field.field_id]["prefix_len"] = key_field.lpm.prefix_len
            elif key_field.HasField("range"):
                key_dict[key_field.field_id]["low"] = key_field.range.low
                key_dict[key_field.field_id]["high"] = key_field.range.high

    def parseDataField(self, field, data_dict):
        '''
        Parse a DataField
        '''
        data_dict.setdefault(field.field_id, [])
        if field.HasField("stream"):
            data_dict[field.field_id].append(field.stream)
        elif field.HasField("str_val"):
            data_dict[field.field_id].append(field.str_val)
        elif field.HasField("bool_val"):
            data_dict[field.field_id].append(field.bool_val)
        elif field.HasField("int_arr_val"):
            int_list = []
            for val in field.int_arr_val.val:
                int_list.append(val)
            data_dict[field.field_id].append(int_list)
        elif field.HasField("bool_arr_val"):
            bool_list = []
            for val in field.bool_arr_val.val:
                bool_list.append(val)
            data_dict[field.field_id].append(bool_list)
        elif field.HasField("container_arr_val"):
            # DataField objects could be encapsulated within a DataField object.
            # Parse the inner dataField objects here.
            for container in field.container_arr_val.container:
                data_fields_list = {}
                for val in container.val:
                    self.parseDataField(val, data_fields_list)
                data_dict[field.field_id].append(data_fields_list)
        else:
            data_dict[field.field_id].append(field.float_val)

    def parseData(self, data, data_dict):
        data_dict["action_id"] = [data.action_id]
        for field in data.fields:
            self.parseDataField(field, data_dict)

        """
        If only one element is present in the list created above,
        then make it into a value
        so now possible (key, values) can be
        (0, [True, False, True]) # for selector member status
        (1, [3,4,1,6]) #4 values in a list for 4 pipes in register read
        (2, 40) #A uint32_t value read
        .
        .
        """
        for key, val in six.iteritems(data_dict):
            if (len(val) == 1):
                data_dict[key] = data_dict[key][0]

    def parseEntryGetResponse(self, response, key_dict = None):
        for rep in response:
            for entity in rep.entities:
                data_dict = {}
                key = entity.table_entry.key
                if not entity.table_entry.is_default_entry:
                    if (key_dict is not None):
                        self.parseKey(key, key_dict)
                data = entity.table_entry.data
                self.parseData(data, data_dict)

                if entity.table_entry.is_default_entry:
                    data_dict["is_default_entry"] = True
                # Yielding here allows to iterate over more entities
                yield data_dict

    def set_table_key(self, table, key_fields, table_name):
        """ Sets the key for a bfn::TableEntry object
            @param table : bfn::TableEntry object.
            @param key_fields: List of (name, value, [mask]) tuples.
        """
        if table is None:
            logger.warning("Invalid TableEntry object.")
            return

        for field in key_fields:
            field_id = self.get_key_field(table_name, field.name)
            if field_id is None:
                logger.error("Data key %s not found.", field.name)
            key_field = table.key.fields.add()
            key_field.field_id = field_id
            if field.mask is not None:
                key_field.ternary.value = field.value
                key_field.ternary.mask = field.mask
            elif field.prefix_len is not None:
                key_field.lpm.value = field.value
                key_field.lpm.prefix_len = field.prefix_len
            elif field.low is not None or field.high is not None:
                key_field.range.low = field.low
                key_field.range.high = field.high
            else:
                key_field.exact.value = field.value

    def set_table_data(self, table, action, data_fields, table_name):
        """ Sets the data for a bfn::TableEntry object
            @param table : bfn::TableEntry object.
            @param ation : Name of the action
            @param data_fields: List of (name, value) tuples.
        """
        if action is not None:
            table.data.action_id = self.get_action(table_name, action)

        if data_fields is not None:
            for field in data_fields:
                data_field = table.data.fields.add()
                data_field.field_id = self.get_data_field(table_name, action, field.name)
                if field.stream is not None:
                    data_field.stream = field.stream
                elif field.float_val is not None:
                    data_field.float_val = field.float_val
                elif field.str_val is not None:
                    data_field.str_val = field.str_val
                elif field.bool_val is not None:
                    data_field.bool_val = field.bool_val
                elif field.int_arr_val is not None:
                    data_field.int_arr_val.val.extend(field.int_arr_val)
                elif field.bool_arr_val is not None:
                    data_field.bool_arr_val.val.extend(field.bool_arr_val)

    def cpy_target(self, req, target_src):
        req.target.device_id = target_src.device_id
        req.target.pipe_id = target_src.pipe_id
        req.target.direction = target_src.direction
        req.target.prsr_id = target_src.prsr_id

    def apply_table_operations(self, target, table_name, table_op):
        """ Apply table operations
            @param target : target device
            @param table_name : Table name.
            @param table_op : table operations to send
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return

        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_operation = update.entity.table_operation
        table_operation.table_id = self.get_table(table_name)
        table_operation.table_operations_type = table_op
        return self.write(req)

    def entry_write_req_make(self, req, table_name,
                           key_fields=[], action_names=[], data_fields=[],
                           update_type=bfruntime_pb2.Update.INSERT,
                           modify_inc_type=None):
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return

        if key_fields is not None and action_names is not None and data_fields is not None:
            assert(len(key_fields) == len(action_names) == len(data_fields));
        for idx in range(len(key_fields)):
            update = req.updates.add()
            update.type = update_type
            table_entry = update.entity.table_entry
            table_entry.table_id = self.get_table(table_name)
            table_entry.is_default_entry = False
            if modify_inc_type != None:
                table_entry.table_mod_inc_flag.type = modify_inc_type

            if key_fields is not None and key_fields[idx] is not None:
                self.set_table_key(table_entry, key_fields[idx], table_name)
            if action_names is not None and data_fields is not None:
                self.set_table_data(table_entry, action_names[idx], data_fields[idx], table_name)
        return req

    def insert_table_entry(self, target, table_name,
                           key_fields=None, action_name=None, data_fields=[]):
        """ Insert a new table entry
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (name, value, [mask]) tuples.
            @param action_name : Action name.
            @param data_fields : List of (name, value) tuples.
        """

        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        return self.write(self.entry_write_req_make(req, table_name, [key_fields],
            [action_name], [data_fields], bfruntime_pb2.Update.INSERT))

    def insert_table_entry_performance(self, target, table_name,
                           key_fields=[], action_names=[], data_fields=[]):
        """ Insert a new table entry
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (List of (name, value, [mask]) tuples).
            @param action_name : List of Action names.
            @param data_fields : List of (List of (name, value) tuples).
        """
        # TODO: This is a temporary function which takes in a list of keyfields, actionnames
        #       and datafields. Moving forward when we restructure this client, we should
        #       remove this API and make insert_table_entry take in a list of all the 
        #       aforementioned things
        assert(len(key_fields) == len(action_names) == len(data_fields));

        req = bfruntime_pb2.WriteRequest()
        req.client_id = self.client_id
        self.cpy_target(req, target)

        try:
            self.stub.Write(self.entry_write_req_make(req, table_name, key_fields,
            action_names, data_fields, bfruntime_pb2.Update.INSERT))
        except grpc.RpcError as e:
            status_code = e.code()
            if status_code != grpc.StatusCode.UNKNOWN:
                logger.info("The error code returned by the server for Performace test is not UNKNOWN, which indicates some error might have occured while trying to add the entries")
                printGrpcError(e)
                raise e
            else:
                # Retrieve the performace rate (entries per second) encoded in the details
                error_details = e.details()
                error_details_list = error_details.split()
                rate = float(error_details_list.pop())
                return rate

    def modify_table_entry(self, target, table_name,
                           key_fields=None, action_name=None, data_fields=None):
        """ Modify a table entry
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (name, value, [mask]) tuples.
            @param action_name : Action name.
            @param data_fields : List of (name, value) tuples.
        """

        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        return self.write(self.entry_write_req_make(req, table_name, [key_fields],
            [action_name], [data_fields], bfruntime_pb2.Update.MODIFY))

    def modify_inc_table_entry(self, target, table_name,
                           key_fields=None, action_name=None, data_fields=None,
                           modify_inc_type=bfruntime_pb2.TableModIncFlag.MOD_INC_ADD):
        """ Modify a table entry
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (name, value, [mask]) tuples.
            @param action_name : Action name.
            @param data_fields : List of (name, value) tuples.
        """

        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        return self.write(self.entry_write_req_make(req, table_name, [key_fields],
            [action_name], [data_fields], bfruntime_pb2.Update.MODIFY_INC, modify_inc_type))

    def set_entry_scope_table_attribute(self, target, table_name, config_gress_scope = False, predefined_gress_scope_val=bfruntime_pb2.Mode.ALL, config_pipe_scope=True, predefined_pipe_scope=True, predefined_pipe_scope_val=bfruntime_pb2.Mode.ALL, user_defined_pipe_scope_val=0xffff, pipe_scope_args=0xff, config_prsr_scope = False, predefined_prsr_scope_val=bfruntime_pb2.Mode.ALL, prsr_scope_args=0xff):
        """ Set Entry Scope for the table
            @param target : target device
            @param table_name : Table name.
            @param config_gress_scope : configure gress_scope for the table
            @param predefined_gress_scope_val : (Optional) Only valid when config_gress_scope=True
            @param config_pipe_scope : configure pipe_scope for the table
            @param predefined_pipe_scope : (Optional) Only valid when config_pipe_scope=True, configure pipe_scope to predefined scope or user_defined one
            @param predefined_pipe_scope_val : (Optional) Only valid when config_pipe_scope=True
            @param user_defined_pipe_scope_val : (Optional) Only valid when pipe_scope type is user defined
            @param pipe_scope_args : (Optional) Only valid when config_pipe_scope=True
            @param config_prsr_scope : configure prsr_scope for the table
            @param predefined_prsr_scope_val : (Optional) Only valid when config_prsr_scope=True
            @param prsr_scope_args : (Optional) Only valid when config_prsr_scope=True
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        if config_gress_scope == True:
            table_attribute.entry_scope.gress_scope.predef = predefined_gress_scope_val
        if config_pipe_scope == True:
            if predefined_pipe_scope == True:
                table_attribute.entry_scope.pipe_scope.predef = predefined_pipe_scope_val
            else:
                table_attribute.entry_scope.pipe_scope.user_defined = user_defined_pipe_scope_val
            table_attribute.entry_scope.pipe_scope.args = pipe_scope_args
        if config_prsr_scope == True:
            table_attribute.entry_scope.prsr_scope.predef = predefined_prsr_scope_val
            table_attribute.entry_scope.prsr_scope.args = prsr_scope_args

        return self.write(req)


    def set_idle_time_table_attribute(self, target, table_name, enable = False, idle_table_mode = bfruntime_pb2.IdleTable.IDLE_TABLE_NOTIFY_MODE, ttl_query_interval = 5000, max_ttl = 3600000, min_ttl = 1000):
        """ Set Entry Scope for the table
            @param target : target device
            @param table_name : Table name.
            @param idle_table_mode : Mode of the idle table (POLL_MODE or NOTIFY_MODE)
            @param ttl_query_length : Minimum query interval 
            @param max_ttl : Max TTL any entry in this table can have in msecs
            @param min_ttl : Min TTL any entry in this table can have in msecs
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        table_attribute.idle_table.enable = enable
        table_attribute.idle_table.idle_table_mode = idle_table_mode
        table_attribute.idle_table.ttl_query_interval = ttl_query_interval
        table_attribute.idle_table.max_ttl = max_ttl
        table_attribute.idle_table.min_ttl = min_ttl

        return self.write(req)

    def set_port_status_change_attribute(self, target, table_name, enable = False):
        """ Set port status change notification for the table
            @param target : target device
            @param table_name : Table name.
            @param enable : notification enable
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        table_attribute.port_status_notify.enable = enable
        return self.write(req)

    def set_port_stat_poll_intvl(self, target, table_name, intvl):
        """ Set port stat poll interval(ms) for the table
            @param target : target device
            @param table_name : Table name.
            @param intvl : time interval, millisecond
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add() 
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        table_attribute.intvl_ms.intvl_val = intvl
        return self.write(req)

    def set_pre_device_config_attribute(self, target, table_name, global_rid=None, port_protection_enable=None, fast_failover_enable=None, max_nodes_before_yield=None, max_node_threshold_node_count=None, max_node_threshold_port_lag_count=None):
        """ Set device config for the PRE MGID table
            @param target : Target device
            @param table_name : Table name.
            @param global_rid : Global RID value
            @param port_protection_enable : True of False to denote port protection enable/disable
            @param fast_failover_enable : True of False to denote fast failover enable/disable
            @param max_nodes_before_yield : max nodes before yield count value
            @param max_node_threshold_node_count : max node threshold node count value
            @param max_node_threshold_port_lag_count : max node threshold port lag count value
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        if global_rid != None:
            table_attribute.pre_device_config.pre_global_rid.global_rid = global_rid

        if port_protection_enable != None:
            table_attribute.pre_device_config.pre_port_protection.enable = port_protection_enable

        if fast_failover_enable != None:
            table_attribute.pre_device_config.pre_fast_failover.enable = fast_failover_enable

        if max_nodes_before_yield != None:
            table_attribute.pre_device_config.pre_max_nodes_before_yield.count = max_nodes_before_yield

        # Either both max_node_threshold_node_count and max_node_threshold_port_lag_count
        # should be present OR both should be absent
        if max_node_threshold_node_count != None and max_node_threshold_port_lag_count == None:
            assert False
        if max_node_threshold_node_count == None and max_node_threshold_port_lag_count != None:
            assert False

        if max_node_threshold_node_count != None:
            table_attribute.pre_device_config.pre_max_node_threshold.node_count = max_node_threshold_node_count
            table_attribute.pre_device_config.pre_max_node_threshold.port_lag_count = max_node_threshold_port_lag_count

        return self.write(req)

    def set_dyn_key_mask_table_attribute(self, target, table_name, key_fields):
        """ Set dynamic key mask for the exact match table
            @param target : target device
            @param table_name : Table name.
            @param key_fields List of (name, mask) tuples.
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)
        for field in key_fields:
            field_id = self.get_key_field(table_name, field.name)
            if field_id is None:
                logger.error("Data key %s not found.", field.name)
            key_field = table_attribute.dyn_key_mask.fields.add()
            key_field.field_id = field_id
            key_field.mask = field.value;

        return self.write(req)

    def set_dyn_hashing_table_attribute(self, target, table_name, alg_hdl = 0, seed = 0):
        """ Set dynamic hashing attribute (algorithm_handler and seed) for the dynamic hashing table
            @param target : target device
            @param table_name : Table name.
            @param alg_hdl: algorithm handler 
            @param seed: seed 
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        table_attribute.dyn_hashing.alg = alg_hdl;
        table_attribute.dyn_hashing.seed = seed;

        return self.write(req)

    def set_meter_bytecount_adjust_attribute(self, target, table_name, byte_count = 0):
        """ Set meter bytecount adjust attribute for the meter table
            @param target : target device
            @param table_name : Table name.
            @param byte_count : number of adjust bytes 
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.INSERT

        table_attribute = update.entity.table_attribute
        table_attribute.table_id = self.get_table(table_name)

        table_attribute.byte_count_adj.byte_count_adjust = byte_count;

        return self.write(req)

    def get_table_entry(self, target, table_name,
                        key_fields, flag_dict, action_name=None, data_field_name_list=None,
                        default_entry=False):
        """ Get a table entry
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (name, value, [mask]) tuples.
            @param flag : dict of flags
            @param action_name : Action name.
            @param data_field_ids : List of field_names
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return

        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        self.cpy_target(req, target)
        return self.read(self.entry_read_req_make(req, table_name, key_fields, flag_dict,
            action_name, data_field_name_list, default_entry))

    def entry_read_req_make(self, req, table_name, key_fields,
            flag_dict, action_name=None, data_field_name_list=None, default_entry=False):
        table_entry = req.entities.add().table_entry
        table_entry.table_id = self.get_table(table_name)
        table_entry.is_default_entry = default_entry

        for key, value in six.iteritems(flag_dict):
            if (key == "from_hw"):
                table_entry.table_read_flag.from_hw = value;

        if (key_fields):
            self.set_table_key(table_entry, key_fields, table_name)

        # We Do not care about values in the data_fields which we are constructing
        data_fields = [self.DataField(field_name, '') for field_name in data_field_name_list or []]
        self.set_table_data(table_entry, action_name, data_fields, table_name)
        return req


    def get_table_usage(self, target, table_name):
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return
        req = bfruntime_pb2.ReadRequest()
        req.client_id = self.client_id;
        self.cpy_target(req, target)

        table_usage = req.entities.add().table_usage
        table_usage.table_id = self.get_table(table_name)

        return self.read(req)

    def delete_table_entry_performance(self, target, table_name,
                           key_fields=[]):
        """ Delete table entries
            @param target : target device
            @param table_name : Table name.
            @param key_fields : List of (List of (name, value, [mask]) tuples).
        """
        # TODO: This is a temporary function which takes in a list of keyfields, actionnames
        #       and datafields. Moving forward when we restructure this client, we should
        #       remove this API and make delete_table_entry take in a list of all the 
        #       aforementioned things

        req = bfruntime_pb2.WriteRequest()
        req.client_id = self.client_id
        self.cpy_target(req, target)

        try:
            self.stub.Write(self.entry_write_req_make(req, table_name, key_fields,
            None, None, bfruntime_pb2.Update.DELETE))
        except grpc.RpcError as e:
            status_code = e.code()
            if status_code != grpc.StatusCode.UNKNOWN:
                logger.info("The error code returned by the server for Performace test is not UNKNOWN, which indicates some error might have occured while trying to delete the entries")
                printGrpcError(e)
                raise e
            else:
                # Retrieve the performace rate (entries per second) encoded in the details
                error_details = e.details()
                error_details_list = error_details.split()
                rate = float(error_details_list.pop())
                return rate

    def delete_table_entry(self, target, table_name, key_fields=None):
        """ Delete a table entry
            @param target : target device
            @param table : Table name.
            @param key_fields: List of (name, value, [mask]) tuples.
        """

        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        return self.write(self.entry_write_req_make(req, table_name, [key_fields],
            [None], [None], bfruntime_pb2.Update.DELETE))

    def reset_table_default_entry(self, target, table_name):
        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)
        update = req.updates.add()
        update.type = bfruntime_pb2.Update.DELETE
        table_entry = update.entity.table_entry
        table_entry.table_id = self.get_table(table_name)
        table_entry.is_default_entry = True
        return self.write(req)

    def modify_table_default_entry(self, target, table_name,
                                   action_name=None, data_fields=None):
        """ Add default entry
            @param target : target device
            @param table_name : Table name.
            @param action_name : Action name.
            @param data_fields : List of (name, value) tuples.
        """
        if self.get_table(table_name) is None:
            logger.warning("Table %s not found", table_name)
            return

        req = bfruntime_pb2.WriteRequest()
        self.cpy_target(req, target)

        update = req.updates.add()
        update.type = bfruntime_pb2.Update.MODIFY

        table_entry = update.entity.table_entry
        table_entry.table_id = self.get_table(table_name)
        table_entry.is_default_entry = True

        self.set_table_data(table_entry, action_name, data_fields, table_name)
        rep = self.write(req)

    def to_bytes(self, n, length):
        """ Conver integers to bytearray. """
        h = '%x' % n
        s = binascii.unhexlify(('0'*(len(h) % 2) + h).zfill(length*2))
        return s

    def ipv4_to_bytes(self, addr):
        """ Convert Ipv4 address to a bytearray. """
        return socket.inet_pton(socket.AF_INET, addr)

    def ipv6_to_bytes(self, addr):
        """ Convert Ipv6 address to a bytearray. """
        return socket.inet_pton(socket.AF_INET6, addr)

    def mac_to_bytes(self, addr):
        """ Covert Mac address to a bytearray. """
        val = map(lambda v: binascii.unhexlify(v), addr.split(':'))
        return b"".join(val)

    def generate_random_ip_list(self, num_entries, seed):
        """ Generate random, unique, non overalapping IP address/mask """
        unique_keys = {}
        ip_list = []
        i = 0
        random.seed(seed)
        duplicate = False
        min_mask_len = max(1, int(math.ceil(math.log(num_entries,2))))
        while( i < num_entries) :
            duplicate = False
            ip = "%d.%d.%d.%d" % (random.randint(1,255), random.randint(0,255), random.randint(0,255), random.randint(0,255))
            p_len = random.randint(min_mask_len,32)
            # Check if the dst_ip, p_len is already present in the list
            ipAddrbytes = ip.split('.')
            ipnumber = (int(ipAddrbytes[0]) << 24) + (int(ipAddrbytes[1]) << 16) + (int(ipAddrbytes[2]) << 8) + int(ipAddrbytes[3])
            mask = 0xffffffff
            mask = (mask << (32 - p_len)) & (0xffffffff)
            if ipnumber & mask in unique_keys:
                continue
            for _, each in six.iteritems(unique_keys):
                each_ip = each[0]
                each_mask = each[1]
                if ipnumber & each_mask == each_ip & each_mask:
                    duplicate = True
                    break
            if duplicate:
                continue
            duplicate = False
            unique_keys[ipnumber & mask] = (ipnumber, mask)
            ip_list.append(self.IpRandom(ip, p_len, mask))
            i += 1
        return ip_list

    def generate_random_mac_list(self, num_entries, seed):
        """ Generate random, unique, non overalapping MAC address/mask """
        unique_keys = {}
        mac_list = []
        i = 0
        random.seed(seed)
        duplicate = False
        while( i < num_entries) :
            duplicate = False
            mac = "%02x:%02x:%02x:%02x:%02x:%02x" % (random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255))
            mask = "%02x:%02x:%02x:%02x:%02x:%02x" % (random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255), random.randint(0,255))
            # Check if the dst_ip, p_len is already present in the list
            macAddrBytes = mac.split(':')
            macMaskBytes = mask.split(":")

            macnumber = 0
            masknumber = 0

            for x in range(len(macAddrBytes)):
                macnumber = macnumber | int(macAddrBytes[x], 16) << (8 * (len(macAddrBytes) - x - 1))
                masknumber = masknumber | int(macAddrBytes[x], 16) << (8 * (len(macAddrBytes) - x - 1))

            if macnumber & masknumber in unique_keys:
                continue

            for _, each in six.iteritems(unique_keys):
                each_mac = each[0]
                each_mask = each[1]
                if macnumber & each_mask == each_mac & each_mask:
                    duplicate = True
                    break
            if duplicate:
                continue
            duplicate = False

            unique_keys[macnumber & masknumber] = (macnumber, masknumber)
            mac_list.append(self.MacRandom(mac, mask))
            i += 1
        return mac_list
