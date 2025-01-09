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


#ifndef SRC_REPORT_ERROR_H_
#define SRC_REPORT_ERROR_H_

#include "google/rpc/code.pb.h"
#include "google/rpc/status.pb.h"

namespace bf {

namespace p4rt {

template <typename Arg1, typename... Args>
static inline ::google::rpc::Status ERROR_STATUS(::google::rpc::Code code,
                                                 const char *fmt,
                                                 const Arg1 &arg1,
                                                 const Args &... /* args */) {
  ::google::rpc::Status status;
  status.set_code(code);
  // TODO: format and set message
  return status;
}

template <typename Arg>
static inline ::google::rpc::Status ERROR_STATUS(::google::rpc::Code code,
                                                 const Arg &msg) {
  ::google::rpc::Status status;
  status.set_code(code);
  status.set_message(msg);
  return status;
}

static inline ::google::rpc::Status ERROR_STATUS(::google::rpc::Code code) {
  ::google::rpc::Status status;
  status.set_code(code);
  return status;
}

static inline ::google::rpc::Status OK_STATUS() {
  ::google::rpc::Status status;
  status.set_code(::google::rpc::Code::OK);
  return status;
}

static inline ::google::rpc::Status GENERIC_STATUS(::google::rpc::Code code) {
  ::google::rpc::Status status;
  status.set_code(code);
  return status;
}

}  // namespace p4rt

}  // namespace bf

#define RETURN_OK_STATUS() return ::bf::p4rt::OK_STATUS();
#define RETURN_ERROR_STATUS(...) return ::bf::p4rt::ERROR_STATUS(__VA_ARGS__);
#define RETURN_STATUS(code) return ::bf::p4rt::GENERIC_STATUS(code);

#define IS_OK(status) (status.code() == ::google::rpc::Code::OK)
#define IS_ERROR(status) (status.code() != ::google::rpc::Code::OK)

#define RETURN_IF_ERROR(status)            \
  do {                                     \
    auto status_ = status;                 \
    if (IS_ERROR(status_)) return status_; \
  } while (false)

#endif  // SRC_REPORT_ERROR_H_
