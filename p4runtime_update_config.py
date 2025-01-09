#!/usr/bin/env python2

import argparse
import errno
from functools import wraps
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import stat
import struct
import sys
import tempfile
import time

import grpc
from p4.v1 import p4runtime_pb2, p4runtime_pb2_grpc
from p4.config.v1 import p4info_pb2
from p4.tmp import p4config_pb2
import google.protobuf.text_format

def get_parser():
    parser = argparse.ArgumentParser(description='Update Tofino P4 config with P4Runtime')
    parser.add_argument('--testdir', help='Location of compiler outputs',
                        type=str, action='store', required=False)
    parser.add_argument('--p4info', help='Path to p4info proto in text format',
                        type=str, action='store', required=False)
    parser.add_argument('--p4-program', '-p', help='Program to upload to the switch',
                        type=str, action='store', required=False)
    parser.add_argument('--name', help='Name of P4 program under test',
                        type=str, action='store', required=False)
    parser.add_argument('--grpc-addr', help='Address to use to connect to '
                        'P4Runtime gRPC server',
                        type=str, action='store',
                        default='localhost:50051')
    parser.add_argument('--switchd-status-port',
                        help='TCP port for checking if switchd is ready',
                        type=int, action='store', required=False)
    parser.add_argument('--arch', choices=['Tofino', 'Tofino2'],
                        default='Tofino')
    return parser

# From
# https://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
class TimeoutError(Exception):
    pass
def timeout(seconds=10, error_message=os.strerror(errno.ETIME)):
    def decorator(func):
        def _handle_timeout(signum, frame):
            raise TimeoutError(error_message)

        def wrapper(*args, **kwargs):
            signal.signal(signal.SIGALRM, _handle_timeout)
            signal.alarm(seconds)
            try:
                result = func(*args, **kwargs)
            finally:
                signal.alarm(0)
            return result

        return wraps(func)(wrapper)

    return decorator

def poll_device(status_port):
    s = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect( ('localhost', status_port) )
        s.sendall('0')
        r = s.recv(1)
        if r == '1':
            return True
        else:
            return False
    except:
        return False
    finally:
        if s is not None:
            s.close()

def wait_for_switchd(status_port, timeout_s=240):
    @timeout(timeout_s)
    def wait():
        while True:
            if poll_device(status_port):
                return
            time.sleep(1)
    print "Waiting for switchd to be ready..."
    try:
        wait()
    except TimeoutError:
        print >> sys.stderr, "Timed out while waiting for switchd to be ready"
        sys.exit(1)

def update_config(name, grpc_addr, p4info_path, tofino_bin_path, cxt_json_path):
    channel = grpc.insecure_channel(grpc_addr)
    stub = p4runtime_pb2_grpc.P4RuntimeStub(channel)

    print "Sending P4 config"
    request = p4runtime_pb2.SetForwardingPipelineConfigRequest()
    request.device_id = 0
    config = request.config
    with open(p4info_path, 'r') as p4info_f:
        google.protobuf.text_format.Merge(p4info_f.read(), config.p4info)
    device_config = p4config_pb2.P4DeviceConfig()
    with open(tofino_bin_path, 'rb') as tofino_bin_f:
        with open(cxt_json_path, 'r') as cxt_json_f:
            device_config.device_data = ""
            prog_name = name
            device_config.device_data += struct.pack("<i", len(prog_name))
            device_config.device_data += prog_name
            tofino_bin = tofino_bin_f.read()
            device_config.device_data += struct.pack("<i", len(tofino_bin))
            device_config.device_data += tofino_bin
            cxt_json = cxt_json_f.read()
            device_config.device_data += struct.pack("<i", len(cxt_json))
            device_config.device_data += cxt_json
    config.p4_device_config = device_config.SerializeToString()
    request.action = p4runtime_pb2.SetForwardingPipelineConfigRequest.VERIFY_AND_COMMIT
    try:
        response = stub.SetForwardingPipelineConfig(request)
    except Exception as e:
        print >> sys.stderr, "Error when trying to push config to bf_switchd"
        print >> sys.stderr, e
        sys.exit(1)
    return True

def exit(msg):
    print >> sys.stderr, msg
    sys.exit(1)

def main():
    args = get_parser().parse_args()

    if args.testdir is None and args.p4info is not None:
        exit("--testdir is required when using --p4info")
    if args.testdir is not None and args.p4info is None:
        exit("--p4info is required when using --testdir")
    if args.p4_program is None and args.testdir is None:
        exit("one of --testdir / --p4-program is required")
    if args.p4_program is not None and args.testdir is not None:
        exit("--testdir / --p4-program are mutually exclusive")

    arch = args.arch.lower()

    if args.p4_program is not None:
        if 'SDE_INSTALL' in os.environ:
            INSTALL = os.environ['SDE_INSTALL']
        else:
            WORKSPACE = os.getenv('WORKSPACE', os.getcwd())
            INSTALL = os.path.join(WORKSPACE, 'install')
        conf_file_path = os.path.join(
            INSTALL, 'share/p4/targets', arch, args.p4_program + '.conf')
        if not os.path.exists(conf_file_path):
            exit("Conf file '{}' not found".format(conf_file_path))
        with open(conf_file_path, 'r') as conf_f:
            conf = json.load(conf_f)
            device = conf['p4_devices'][0]
            p4_programs = device['p4_programs']
            if len(p4_programs) > 1:
                exit("More than one program in conf file is not supported with P4Runtime")
            p4_pipes = p4_programs[0]['p4_pipelines']
            if len(p4_pipes) > 1:
                exit("More than one pipeline in conf file is not supported with P4Runtime")
            config_path = os.path.join(INSTALL, p4_pipes[0]['config'])
            context_path = os.path.join(INSTALL, p4_pipes[0]['context'])
            p4info_path = os.path.join(
                INSTALL, p4_pipes[0]['path'], 'p4info.pb.txt')
    else:
        compiler_out_dir = args.testdir
        config_path = os.path.join(compiler_out_dir, 'tofino.bin')
        context_path = os.path.join(compiler_out_dir, 'context.json')
        p4info_path = args.p4info

    if not os.path.exists(p4info_path):
        exit("P4info text protobuf file '{}' not found".format(p4info_path))

    if not os.path.exists(config_path):
        exit("Binary config file '{}' not found".format(config_path))
    if not os.path.exists(context_path):
        exit("Context JSON file '{}' not found".format(context_path))

    if args.switchd_status_port is not None:
        wait_for_switchd(args.switchd_status_port)

    if args.name is not None:
        name = args.name
    elif args.p4_program is not None:
        name = args.p4_program
    else:
        name = "program"

    update_config(name, args.grpc_addr, p4info_path, config_path, context_path)

if __name__ == '__main__':
    main()
