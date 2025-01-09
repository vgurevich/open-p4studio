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


#include "google/rpc/code.pb.h"
#include "google/rpc/status.pb.h"

#include "p4_rt/device_mgr.h"

#include "report_error.h"

using Code = ::google::rpc::Code;
using device_id_t = ::pi::fe::proto::DeviceMgr::device_id_t;
using p4_id_t = ::pi::fe::proto::DeviceMgr::p4_id_t;
using Status = ::pi::fe::proto::DeviceMgr::Status;
using StreamMessageResponseCb =
    ::pi::fe::proto::DeviceMgr::StreamMessageResponseCb;

namespace p4v1 = ::p4::v1;
namespace p4configv1 = ::p4::config::v1;

namespace bf {

namespace p4rt {

// TODO: implement with BFRT
class DeviceMgrImp {
 public:
  explicit DeviceMgrImp(device_id_t device_id) : device_id(device_id) {}

  ~DeviceMgrImp() {}

  Status pipeline_config_set(
      p4v1::SetForwardingPipelineConfigRequest::Action /*action*/,
      const p4v1::ForwardingPipelineConfig & /*config*/) {
    RETURN_ERROR_STATUS(Code::UNIMPLEMENTED);
  }

  Status pipeline_config_get(
      p4v1::GetForwardingPipelineConfigRequest::ResponseType /*response_type*/,
      p4v1::ForwardingPipelineConfig * /*config*/) {
    RETURN_ERROR_STATUS(Code::UNIMPLEMENTED);
  }

  Status write(const p4v1::WriteRequest & /*request*/) {
    RETURN_ERROR_STATUS(Code::UNIMPLEMENTED);
  }

  Status read(const p4v1::ReadRequest & /*request*/,
              p4v1::ReadResponse * /*response*/) const {
    RETURN_ERROR_STATUS(Code::UNIMPLEMENTED);
  }

  Status read_one(const p4v1::Entity & /*entity*/,
                  p4v1::ReadResponse * /*response*/) const {
    RETURN_ERROR_STATUS(Code::UNIMPLEMENTED);
  }

  Status stream_message_request_handle(
      const p4::v1::StreamMessageRequest & /*request*/) {
    RETURN_ERROR_STATUS(Code::UNIMPLEMENTED);
  }

  void stream_message_response_register_cb(StreamMessageResponseCb /*cb*/,
                                           void * /*cookie*/) {}

  static void init(size_t /*max_devices*/) {}

  static void destroy() {}

 private:
  device_id_t device_id;
};

}  // namespace p4rt

}  // namespace bf

namespace pi {

namespace fe {

namespace proto {

// The implementation is in the ::bf::p4rt namespace.
// This is just a proxy
class DeviceMgrImp : public ::bf::p4rt::DeviceMgrImp {
 public:
  template <typename... Args>
  DeviceMgrImp(Args &&... args)
      : ::bf::p4rt::DeviceMgrImp(std::forward<Args>(args)...) {}
};

DeviceMgr::DeviceMgr(device_id_t device_id) {
  pimp = std::unique_ptr<DeviceMgrImp>(new DeviceMgrImp(device_id));
}

DeviceMgr::~DeviceMgr() {}

// PIMPL forwarding

Status DeviceMgr::pipeline_config_set(
    p4v1::SetForwardingPipelineConfigRequest::Action action,
    const p4v1::ForwardingPipelineConfig &config) {
  return pimp->pipeline_config_set(action, config);
}

Status DeviceMgr::pipeline_config_get(
    p4v1::GetForwardingPipelineConfigRequest::ResponseType response_type,
    p4v1::ForwardingPipelineConfig *config) {
  return pimp->pipeline_config_get(response_type, config);
}

Status DeviceMgr::write(const p4v1::WriteRequest &request) {
  return pimp->write(request);
}

Status DeviceMgr::read(const p4v1::ReadRequest &request,
                       p4v1::ReadResponse *response) const {
  return pimp->read(request, response);
}

Status DeviceMgr::read_one(const p4v1::Entity &entity,
                           p4v1::ReadResponse *response) const {
  return pimp->read_one(entity, response);
}

Status DeviceMgr::stream_message_request_handle(
    const p4::v1::StreamMessageRequest &request) {
  return pimp->stream_message_request_handle(request);
}

void DeviceMgr::stream_message_response_register_cb(StreamMessageResponseCb cb,
                                                    void *cookie) {
  return pimp->stream_message_response_register_cb(std::move(cb), cookie);
}

void DeviceMgr::init(size_t max_devices) { DeviceMgrImp::init(max_devices); }

void DeviceMgr::destroy() { DeviceMgrImp::destroy(); }

}  // namespace proto

}  // namespace fe

}  // namespace pi
