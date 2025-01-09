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


#include <bf_rt_pre/bf_rt_mc_mgr_intf.hpp>
#include <bf_rt_tm/bf_rt_tm_intf.hpp>
#include "bf_rt_pipe_mgr_intf.hpp"
#include "bf_rt_session_impl.hpp"
#include "bf_rt_utils.hpp"

#include "bf_rt_init_impl.hpp"

namespace bfrt {

std::shared_ptr<BfRtSession> BfRtSession::sessionCreate() {
  auto session = std::make_shared<BfRtSessionImpl>();
  auto status = session->sessionCreateInternal();
  if (status != BF_SUCCESS) {
    return nullptr;
  }
  return session;
}

BfRtSession *BfRtSession::sessionInstantiate(bf_rt_id_t session_id,
                                             bf_rt_id_t mc_session_id) {
  auto session = new BfRtSessionImpl(session_id, mc_session_id);
  return session;
}

bf_status_t BfRtSessionImpl::sessionCreateInternal() {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  pipe_sess_hdl_t sessHndl = 0;
  auto *mcMgr = McMgrIntf::getInstance();
  bf_mc_session_hdl_t mcSessHndl = 0;

  bool pkt_mgr_skip;
  bool mc_mgr_skip;
  bool port_mgr_skip;
  bool traffic_mgr_skip;
  BfRtDevMgrImpl::bfRtDeviceConfigGet(
      &pkt_mgr_skip, &mc_mgr_skip, &port_mgr_skip, &traffic_mgr_skip);

  status = pipeMgr->pipeMgrClientInit(&sessHndl);
  if (status != BF_SUCCESS) {
    LOG_ERROR("%s:%d Error in creating pipe mgr session, err %s",
              __func__,
              __LINE__,
              pipe_str_err((pipe_status_t)status));
    return status;
  }
  session_handle_ = sessHndl;

  if (!mc_mgr_skip) {
    status = mcMgr->mcMgrCreateSession(&mcSessHndl);
    if (status != BF_SUCCESS) {
      LOG_ERROR("%s:%d Error in creating mc mgr session, err %s",
                __func__,
                __LINE__,
                bf_err_str(status));
      // Since the mc mgr session creation failed, delete pipe mgr
      // session and return error
      bf_status_t temp_sts = pipeMgr->pipeMgrClientCleanup(sessHndl);
      if (temp_sts != BF_SUCCESS) {
        LOG_ERROR(
            "%s:%d Error in cleaning up the pipe mgr session "
            "with handle %d, err %s",
            __func__,
            __LINE__,
            sessHndl,
            pipe_str_err((pipe_status_t)temp_sts));
      }
      // return the original error status
      return status;
    }
    pre_session_handle_ = mcSessHndl;
  }

  // Mark session as valid
  valid_ = true;

  return BF_SUCCESS;
}

BfRtSessionImpl::BfRtSessionImpl()
    : in_pipe_mgr_batch_(false),
      in_mc_mgr_batch_(false),
      in_batch_(false),
      session_handle_(),
      pre_session_handle_(),
      valid_(false) {}

BfRtSessionImpl::BfRtSessionImpl(bf_rt_id_t sess_id, bf_rt_id_t mc_sess_id)
    : in_pipe_mgr_batch_(false),
      in_mc_mgr_batch_(false),
      in_batch_(false),
      session_handle_(sess_id),
      pre_session_handle_(mc_sess_id),
      valid_(false) {}

BfRtSessionImpl::~BfRtSessionImpl() {
  // session delete here
  if (valid_) {
    bf_status_t status = sessionDestroy();
    (void)status;
  }
}

bf_status_t BfRtSessionImpl::sessionDestroy() {
  bf_status_t status = BF_SUCCESS;
  auto *pipeMgr = PipeMgrIntf::getInstance();
  auto *mcMgr = McMgrIntf::getInstance();

  bool pkt_mgr_skip;
  bool mc_mgr_skip;
  bool port_mgr_skip;
  bool traffic_mgr_skip;
  BfRtDevMgrImpl::bfRtDeviceConfigGet(
      &pkt_mgr_skip, &mc_mgr_skip, &port_mgr_skip, &traffic_mgr_skip);

  status = pipeMgr->pipeMgrClientCleanup(session_handle_);
  if (status != BF_SUCCESS) {
    LOG_ERROR(
        "%s:%d Error in cleaning up the pipe mgr session "
        "with handle %d, err %s",
        __func__,
        __LINE__,
        session_handle_,
        pipe_str_err((pipe_status_t)status));
    return status;
  }
  this->in_pipe_mgr_batch_ = false;

  if (!mc_mgr_skip) {
    status = mcMgr->mcMgrDestroySession(pre_session_handle_);
    if (status != BF_SUCCESS) {
      LOG_ERROR(
          "%s:%d Error in cleaning up the mc mgr session "
          "with handle %d, err %s",
          __func__,
          __LINE__,
          pre_session_handle_,
          bf_err_str(status));
      return status;
    }
    this->in_mc_mgr_batch_ = false;
  }
  this->in_batch_ = false;
  // Mark session as invalid
  valid_ = false;

  return BF_SUCCESS;
}

bf_status_t BfRtSessionImpl::sessionCompleteOperations() const {
  if (valid_) {
    bf_status_t status_ = BF_SUCCESS;
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();
    auto *mcMgr = McMgrIntf::getInstance();
    auto *trafficMgr = TrafficMgrIntf::getInstance();

    bool pkt_mgr_skip;
    bool mc_mgr_skip;
    bool port_mgr_skip;
    bool traffic_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(
        &pkt_mgr_skip, &mc_mgr_skip, &port_mgr_skip, &traffic_mgr_skip);

    status = pipeMgr->pipeMgrCompleteOperations(session_handle_);
    if (BF_SUCCESS != status) {
      status_ = status;
      LOG_ERROR("%s:%d Pipe Manager complete operations failed err %s",
                __func__,
                __LINE__,
                bf_err_str(status));
    }

    if (!mc_mgr_skip) {
      status = mcMgr->mcMgrCompleteOperations(pre_session_handle_);
      if (BF_SUCCESS != status) {
        status_ = status;
        LOG_ERROR("%s:%d Multicast Manager complete operations failed err %s",
                  __func__,
                  __LINE__,
                  bf_err_str(status));
      }
    }

    if (!traffic_mgr_skip) {
      std::set<bf_dev_id_t> device_id_list;
      BfRtDevMgr &devMgr = BfRtDevMgr::getInstance();
      status = devMgr.bfRtDeviceIdListGet(&device_id_list);
      if (BF_SUCCESS == status) {
        for (const auto &dev_id : device_id_list) {
          status = trafficMgr->bfTMCompleteOperations(dev_id);
          if (BF_INVALID_ARG == status) {
            LOG_TRACE("%s:%d Traffic Manager context for dev_id=%d isn't ready",
                      __func__,
                      __LINE__,
                      dev_id);
            continue;
          }
          if (BF_SUCCESS != status) {
            status_ = status;
            LOG_ERROR(
                "%s:%d Traffic Manager dev_id=%d complete operations failed "
                "err %s",
                __func__,
                __LINE__,
                dev_id,
                bf_err_str(status));
          } else {
            LOG_DBG("%s:%d Traffic Manager dev_id=%d complete operations",
                    __func__,
                    __LINE__,
                    dev_id);
          }
        }
      } else {
        status_ = status;
      }
    }

    return status_;
  }
  return BF_INVALID_ARG;
}

// Batching functions
bf_status_t BfRtSessionImpl::beginBatch() const {
  if (valid_ && !this->in_batch_) {
    this->in_batch_ = true;
    bool pkt_mgr_skip;
    bool mc_mgr_skip;
    bool port_mgr_skip;
    bool traffic_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(
        &pkt_mgr_skip, &mc_mgr_skip, &port_mgr_skip, &traffic_mgr_skip);
    if (mc_mgr_skip) {
      // Will cause start batch to be ignored.
      this->in_mc_mgr_batch_ = true;
    }

    return BF_SUCCESS;
  }
  LOG_ERROR(
      "%s:%d Session is invalid or already in batch.", __func__, __LINE__);
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::flushBatch() const {
  if (valid_) {
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();
    auto *mcMgr = McMgrIntf::getInstance();
    bool pkt_mgr_skip;
    bool mc_mgr_skip;
    bool port_mgr_skip;
    bool traffic_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(
        &pkt_mgr_skip, &mc_mgr_skip, &port_mgr_skip, &traffic_mgr_skip);

    if (this->in_pipe_mgr_batch_) {
      status = pipeMgr->pipeMgrFlushBatch(session_handle_);
    }

    if (!mc_mgr_skip && this->in_mc_mgr_batch_) {
      status |= mcMgr->mcMgrFlushBatch(pre_session_handle_);
    }

    return status;
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::endBatch(bool hwSynchronous) const {
  if (valid_) {
    bf_status_t status = BF_SUCCESS;
    auto *pipeMgr = PipeMgrIntf::getInstance();
    auto *mcMgr = McMgrIntf::getInstance();

    bool pkt_mgr_skip;
    bool mc_mgr_skip;
    bool port_mgr_skip;
    bool traffic_mgr_skip;
    BfRtDevMgrImpl::bfRtDeviceConfigGet(
        &pkt_mgr_skip, &mc_mgr_skip, &port_mgr_skip, &traffic_mgr_skip);

    if (this->in_pipe_mgr_batch_) {
      status = pipeMgr->pipeMgrEndBatch(session_handle_, hwSynchronous);
      this->in_pipe_mgr_batch_ = false;
    }

    if (!mc_mgr_skip && this->in_mc_mgr_batch_) {
      status |= mcMgr->mcMgrEndBatch(pre_session_handle_, hwSynchronous);
    }
    this->in_mc_mgr_batch_ = false;

    this->in_batch_ = false;
    return status;
  }
  return BF_INVALID_ARG;
}

// Transaction functions
bf_status_t BfRtSessionImpl::beginTransaction(bool isAtomic) const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrBeginTxn(session_handle_, isAtomic);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::verifyTransaction() const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrVerifyTxn(session_handle_);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::commitTransaction(bool hwSynchronous) const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrCommitTxn(session_handle_, hwSynchronous);
  }
  return BF_INVALID_ARG;
}

bf_status_t BfRtSessionImpl::abortTransaction() const {
  if (valid_) {
    auto *pipeMgr = PipeMgrIntf::getInstance();
    return pipeMgr->pipeMgrAbortTxn(session_handle_);
  }
  return BF_INVALID_ARG;
}

}  // namespace bfrt
