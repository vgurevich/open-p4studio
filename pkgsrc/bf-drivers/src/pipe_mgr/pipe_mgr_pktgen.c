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


#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <pipe_mgr/pktgen_intf.h>
#include "pipe_mgr_int.h"
#include "pipe_mgr_drv.h"
#include "pipe_mgr_pktgen.h"
#include "pipe_mgr_tof_pktgen.h"
#include "pipe_mgr_tof2_pktgen.h"
#include "pipe_mgr_tof3_pktgen.h"
#include "pipe_mgr_pktgen_comm.h"

/* Pointer to global pipe_mgr context */
extern pipe_mgr_ctx_t *pipe_mgr_ctx;

#define PG_API_START(shdl, d)                                                 \
  if (PIPE_SUCCESS != pipe_mgr_api_enter(shdl)) {                             \
    return BF_SESSION_NOT_FOUND;                                              \
  } else {                                                                    \
    if (pipe_mgr_sess_in_batch(shdl)) {                                       \
      pipe_mgr_drv_ilist_chkpt(shdl);                                         \
    }                                                                         \
    pipe_status_t verify_sts = pipe_mgr_verify_pkt_gen_access(shdl, d, true); \
    if (PIPE_SUCCESS != verify_sts) {                                         \
      pipe_mgr_api_exit(shdl);                                                \
      return verify_sts;                                                      \
    }                                                                         \
  }
#define PG_API_END(shdl, sts, batchable)                                     \
  if (BF_SUCCESS != sts) {                                                   \
    LOG_ERROR("%s returned status %s", __func__, bf_err_str(sts));           \
    if (pipe_mgr_sess_in_txn(shdl)) {                                        \
      pipe_mgr_abort_txn_int(shdl);                                          \
    } else if (pipe_mgr_sess_in_batch(shdl)) {                               \
      pipe_mgr_drv_ilist_rollback(shdl);                                     \
    } else {                                                                 \
      pipe_mgr_sm_release(shdl);                                             \
    }                                                                        \
  } else if (!pipe_mgr_sess_in_txn(shdl) && !pipe_mgr_sess_in_batch(shdl)) { \
    pipe_mgr_sm_release(shdl);                                               \
    pipe_status_t x_ = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);           \
    if (PIPE_SUCCESS != x_) {                                                \
      sts = BF_HW_COMM_FAIL;                                                 \
    }                                                                        \
  } else if (!batchable && pipe_mgr_sess_in_batch(shdl)) {                   \
    pipe_status_t x_ = pipe_mgr_drv_ilist_push(&shdl, NULL, NULL);           \
    if (PIPE_SUCCESS != x_) {                                                \
      sts = BF_HW_COMM_FAIL;                                                 \
    }                                                                        \
  }                                                                          \
  pipe_mgr_api_exit(shdl);

static bool illegal_app_id(bf_dev_id_t dev, int app_id) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return true;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return ((0 > app_id) || (PIPE_MGR_TOF_PKTGEN_APP_CNT <= app_id));
    case BF_DEV_FAMILY_TOFINO2:
      return ((0 > app_id) || (PIPE_MGR_TOF2_PKTGEN_APP_CNT <= app_id));
    case BF_DEV_FAMILY_TOFINO3:
      return ((0 > app_id) || (PIPE_MGR_TOF3_PKTGEN_APP_CNT <= app_id));
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return true;
  }
}
static bool pktgen_valid_dev_tgt(bf_dev_target_t x,
                                 const char *where,
                                 const int line) {
  if (!pipe_mgr_valid_deviceId(x.device_id, where, line)) return false;
  if (x.dev_pipe_id != BF_DEV_PIPE_ALL) {
    unsigned int a = pipe_mgr_get_num_active_pipes(x.device_id);
    if (x.dev_pipe_id >= a) {
      LOG_ERROR("Invalid pipe (%d.%d, count %d) specified at %s:%d",
                x.device_id,
                x.dev_pipe_id,
                a,
                where,
                line);
      return false;
    }
  }
  return true;
}

uint32_t pipe_mgr_pktgen_get_app_count(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return PIPE_MGR_TOF_PKTGEN_APP_CNT;
    case BF_DEV_FAMILY_TOFINO2:
      return PIPE_MGR_TOF2_PKTGEN_APP_CNT;
    case BF_DEV_FAMILY_TOFINO3:
      return PIPE_MGR_TOF3_PKTGEN_APP_CNT;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return 0;
  }
}
bf_status_t pipe_mgr_pktgen_add_dev(bf_session_hdl_t shdl, bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return BF_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_pktgen_add_dev(shdl, dev);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_add_dev(shdl, dev);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_add_dev(shdl, dev);





    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return BF_UNEXPECTED;
  }
}

bf_status_t pipe_mgr_pktgen_rmv_dev(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return BF_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_pktgen_rmv_dev(dev);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_rmv_dev(dev);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_rmv_dev(dev);
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return BF_UNEXPECTED;
  }
}

bf_status_t pipe_mgr_pktgen_warm_init_quick(bf_session_hdl_t shdl,
                                            bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return BF_INVALID_ARG;

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_pktgen_warm_init_quick(shdl, dev);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_warm_init_quick(shdl, dev);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_warm_init_quick(shdl, dev);
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return BF_UNEXPECTED;
  }
}

bf_status_t pipe_mgr_pktgen_port_add(rmt_dev_info_t *dev_info,
                                     bf_dev_port_t port_id,
                                     bf_port_speeds_t speed) {
  if (!dev_info) return BF_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_pktgen_port_add(dev_info, port_id, speed);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_port_add(dev_info, port_id, speed);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_port_add(dev_info, port_id, speed);




    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_info->dev_id);
      return BF_UNEXPECTED;
  }
}

bf_status_t pipe_mgr_pktgen_port_rem(rmt_dev_info_t *dev_info,
                                     bf_dev_port_t port_id) {
  if (!dev_info) return BF_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_pktgen_port_rem(dev_info, port_id);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_port_rem(dev_info, port_id);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_port_rem(dev_info, port_id);
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_info->dev_id);
      return BF_UNEXPECTED;
  }
}

bf_status_t bf_recirculation_set(bf_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool enable) {
  if (enable) {
    return bf_recirculation_enable(shdl, dev, port);
  } else {
    return bf_recirculation_disable(shdl, dev, port);
  }
}

bf_status_t bf_recirculation_get(bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool *is_enabled) {
  if (!is_enabled) return BF_INVALID_ARG;

  if (!pipe_mgr_ctx) {
    *is_enabled = false;
    return BF_SUCCESS;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info || !dev_info->dev_cfg.dev_port_validate(port)) {
    LOG_ERROR("%s:%d Invalid dev_info or invalid port, dev %d, port %d",
              __func__,
              __LINE__,
              dev,
              port);
    return BF_INVALID_ARG;
  }

  bf_dev_target_t dev_tgt = {dev, dev_info->dev_cfg.dev_port_to_pipe(port)};
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port);
  if (dev_tgt.dev_pipe_id >= pipe_mgr_get_num_active_pipes(dev)) {
    LOG_ERROR("%s:%d Invalid port, dev %d, pipe %d, port %d",
              __func__,
              __LINE__,
              dev,
              dev_tgt.dev_pipe_id,
              port);
    return BF_INVALID_ARG;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (local_port >= 68) {
        /* We enable recirc by default and never turn it off. */
        *is_enabled = true;
      } else if (local_port >= 64) {
        *is_enabled =
            pipe_mgr_tof_pktgen_reg_pgr_com_port16_get_recirc(dev_tgt);
      } else {
        *is_enabled = false;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *is_enabled =
          (local_port < 8) ? pipe_mgr_tof2_pgr_recir_get(dev, port) : 0;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      if ((local_port < 8)) {
        *is_enabled = pipe_mgr_tof3_pgr_recir_get(dev, port);
      } else {
        *is_enabled = false;
      }
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      *is_enabled = false;
      return BF_UNEXPECTED;
  }

  return BF_SUCCESS;
}

bf_status_t bf_recirculation_enable(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port) {
  bool enabled = false;
  bf_status_t sts = bf_recirculation_get(dev, port, &enabled);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Dev %d port %d cannot get recirculation state, %s",
              dev,
              port,
              bf_err_str(sts));
    return sts;
  }
  if (enabled) return BF_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    return sts;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_set_recirc_16_en(shdl, dev, port, true);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_recir_en(shdl, dev, port, true);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_recir_en(shdl, dev, port, true);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, false);
  return sts;
}
bf_status_t bf_recirculation_disable(bf_session_hdl_t shdl,
                                     bf_dev_id_t dev,
                                     bf_dev_port_t port) {
  bool enabled = false;
  bf_status_t sts = bf_recirculation_get(dev, port, &enabled);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Dev %d port %d cannot get recirculation state, %s",
              dev,
              port,
              bf_err_str(sts));
    return sts;
  }
  if (!enabled) return BF_SUCCESS;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    return sts;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_set_recirc_16_en(shdl, dev, port, false);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_recir_en(shdl, dev, port, false);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_recir_en(shdl, dev, port, false);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_enable(bf_session_hdl_t shdl,
                             bf_dev_id_t dev,
                             bf_dev_port_t port) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    return sts;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_set_port_en(shdl, dev, port, true);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_en(shdl, dev, port, true);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_en(shdl, dev, port, true);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, false);
  return sts;
}
bf_status_t bf_pktgen_disable(bf_session_hdl_t shdl,
                              bf_dev_id_t dev,
                              bf_dev_port_t port) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    return sts;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_set_port_en(shdl, dev, port, false);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_en(shdl, dev, port, false);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_en(shdl, dev, port, false);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_enable_set(bf_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool enable) {
  if (enable) {
    return (bf_pktgen_enable(shdl, dev, port));
  } else {
    return (bf_pktgen_disable(shdl, dev, port));
  }
}

bf_status_t bf_pktgen_enable_get(bf_session_hdl_t shdl,
                                 bf_dev_id_t dev,
                                 bf_dev_port_t port,
                                 bool *is_enabled) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return BF_INVALID_ARG;
  }
  if (is_enabled == NULL) return BF_INVALID_ARG;
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_get_port_en(dev_info, port, is_enabled);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_get_port_en(dev_info, port, is_enabled);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_get_port_en(dev_info, port, is_enabled);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_enable_recirc_pattern_matching(bf_session_hdl_t shdl,
                                                     bf_dev_id_t dev,
                                                     bf_dev_port_t port) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    return sts;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_set_rpm_en(shdl, dev, port, true);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      /* Tofino2, it is enabled during app conf */
      sts = BF_SUCCESS;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      /* Tofino3, it is enabled during app conf */
      sts = BF_SUCCESS;
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_disable_recirc_pattern_matching(bf_session_hdl_t shdl,
                                                      bf_dev_id_t dev,
                                                      bf_dev_port_t port) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    return sts;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_set_rpm_en(shdl, dev, port, false);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      /* Tofino2, it is disabled within app conf */
      sts = BF_SUCCESS;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      /* Tofino3, it is disabled within app conf */
      sts = BF_SUCCESS;
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_recirc_pattern_matching_set(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bool enable) {
  if (enable) {
    return bf_pktgen_enable_recirc_pattern_matching(shdl, dev, port);
  } else {
    return bf_pktgen_disable_recirc_pattern_matching(shdl, dev, port);
  }
}

bf_status_t bf_pktgen_recirc_pattern_matching_get(bf_session_hdl_t shdl,
                                                  bf_dev_id_t dev,
                                                  bf_dev_port_t port,
                                                  bool *is_enabled) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    return BF_INVALID_ARG;
  }
  if (is_enabled == NULL) return BF_INVALID_ARG;
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_get_rpm_en(dev_info, port, is_enabled);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = BF_SUCCESS;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = BF_SUCCESS;
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_port_down_get(bf_session_hdl_t shdl,
                                    bf_dev_id_t dev,
                                    bf_dev_port_t port,
                                    bool *is_cleared) {
  bf_status_t sts = BF_SUCCESS;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("Invalid device %d for %s on port 0x%x", dev, __func__, port);
    return BF_INVALID_ARG;
  }
  if (!dev_info->dev_cfg.dev_port_validate(port)) {
    LOG_ERROR(
        "%s:%d Invalid port, dev %d, port 0x%x", __func__, __LINE__, dev, port);
    return BF_INVALID_ARG;
  }
  if (is_cleared == NULL) return BF_INVALID_ARG;

  bf_dev_target_t dev_tgt = {dev, dev_info->dev_cfg.dev_port_to_pipe(port)};
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port);
  if (dev_tgt.dev_pipe_id >= dev_info->num_active_pipes) {
    LOG_ERROR("%s:%d Invalid pipe %d, dev %d, port 0x%x",
              __func__,
              __LINE__,
              dev_tgt.dev_pipe_id,
              dev,
              port);
    return BF_INVALID_ARG;
  }
  if (port < 0 || local_port >= (int)dev_info->dev_cfg.num_ports) {
    LOG_ERROR("%s:%d Invalid port 0x%x, dev %d", __func__, __LINE__, port, dev);
    return BF_INVALID_ARG;
  }
  PG_API_START(shdl, dev);
  if (pipe_mgr_sess_in_txn(shdl)) {
    LOG_ERROR("%s:%d TXN not supported, dev %d, shdl %d",
              __func__,
              __LINE__,
              dev,
              shdl);
    sts = BF_TXN_NOT_SUPPORTED;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_pgr_com_port_down_clr_get(
          dev_tgt, local_port, is_cleared);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_pgr_com_port_down_clr_get(
          dev_tgt, local_port, is_cleared);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_pgr_com_port_down_clr_get(
          shdl, dev_tgt.device_id, dev_tgt.dev_pipe_id, local_port, is_cleared);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      break;
  }
  if (BF_SUCCESS == sts) {
    LOG_TRACE("Dev %d, pktgen get port-down-clear port %d", dev, port);
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_clear_port_down(bf_session_hdl_t shdl,
                                      bf_dev_id_t dev,
                                      bf_dev_port_t port) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev);
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    LOG_ERROR("Invalid device %d for %s on port 0x%x", dev, __func__, port);
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!dev_info->dev_cfg.dev_port_validate(port)) {
    LOG_ERROR(
        "%s:%d Invalid port, dev %d, port 0x%x", __func__, __LINE__, dev, port);
    sts = BF_INVALID_ARG;
    goto done;
  }

  bf_dev_target_t dev_tgt = {dev, dev_info->dev_cfg.dev_port_to_pipe(port)};
  int local_port = dev_info->dev_cfg.dev_port_to_local_port(port);
  if (dev_tgt.dev_pipe_id >= dev_info->num_active_pipes) {
    LOG_ERROR("%s:%d Invalid pipe %d, dev %d, port 0x%x",
              __func__,
              __LINE__,
              dev_tgt.dev_pipe_id,
              dev,
              port);
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (port < 0 || local_port >= (int)dev_info->dev_cfg.num_ports) {
    LOG_ERROR("%s:%d Invalid port 0x%x, dev %d", __func__, __LINE__, port, dev);
    sts = BF_INVALID_ARG;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts =
          pipe_mgr_tof_pktgen_pgr_com_port_down_clr(shdl, dev_tgt, local_port);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts =
          pipe_mgr_tof2_pktgen_pgr_com_port_down_clr(shdl, dev_tgt, local_port);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_pgr_com_port_down_clr(
          shdl, dev_tgt.device_id, dev_tgt.dev_pipe_id, local_port);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      sts = BF_UNEXPECTED;
      break;
  }
  if (BF_SUCCESS == sts) {
    LOG_TRACE("Dev %d, pktgen port-down-clear port %d", dev, port);
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

static bf_status_t pktgen_set_app_enable(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         int app_id,
                                         bool en) {
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) return BF_INVALID_ARG;
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) return BF_INVALID_ARG;
  if (illegal_app_id(dev_tgt.device_id, app_id)) return BF_INVALID_ARG;
  if (dev_tgt.device_id >= PIPE_MGR_NUM_DEVICES || dev_tgt.device_id < 0)
    return BF_INVALID_ARG;
  if ((uint32_t)app_id >= pipe_mgr_pktgen_get_app_count(dev_tgt.device_id))
    return BF_INVALID_ARG;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }

  LOG_TRACE("PktGenAppCfg: dev %d pipe %d app %d shdl %d, Ctrl en %d",
            dev_tgt.device_id,
            dev_tgt.dev_pipe_id,
            app_id,
            shdl,
            en);

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      return pipe_mgr_tof_pktgen_set_app_ctrl_en(shdl, dev_tgt, app_id, en);
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_reg_pgr_app_ctrl_en(
          shdl, dev_tgt, app_id, en);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_reg_pgr_app_ctrl_en(
          shdl, dev_tgt.device_id, dev_tgt.dev_pipe_id, app_id, en);
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      return BF_UNEXPECTED;
  }
}
bf_status_t bf_pktgen_app_enable(bf_session_hdl_t shdl,
                                 bf_dev_target_t dev_tgt,
                                 int app_id) {
  PG_API_START(shdl, dev_tgt.device_id);
  bf_status_t x = pktgen_set_app_enable(shdl, dev_tgt, app_id, true);
  PG_API_END(shdl, x, true);

  return x;
}
bf_status_t bf_pktgen_app_disable(bf_session_hdl_t shdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id) {
  PG_API_START(shdl, dev_tgt.device_id);
  bf_status_t x = pktgen_set_app_enable(shdl, dev_tgt, app_id, false);
  PG_API_END(shdl, x, true);

  return x;
}

bf_status_t bf_pktgen_app_set(bf_session_hdl_t shdl,
                              bf_dev_target_t dev_tgt,
                              int app_id,
                              bool enable) {
  if (enable) {
    return bf_pktgen_app_enable(shdl, dev_tgt, app_id);
  } else {
    return bf_pktgen_app_disable(shdl, dev_tgt, app_id);
  }
}

bf_status_t bf_pktgen_app_get(bf_session_hdl_t shdl,
                              bf_dev_target_t dev_tgt,
                              int app_id,
                              bool *is_enabled) {
  bf_status_t sts = BF_SUCCESS;
  if (is_enabled == NULL) return BF_INVALID_ARG;
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) return BF_INVALID_ARG;
  if (illegal_app_id(dev_tgt.device_id, app_id)) return BF_INVALID_ARG;
  if (dev_tgt.device_id >= PIPE_MGR_NUM_DEVICES || dev_tgt.device_id < 0)
    return BF_INVALID_ARG;
  if ((uint32_t)app_id >= pipe_mgr_pktgen_get_app_count(dev_tgt.device_id))
    return BF_INVALID_ARG;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  PG_API_START(shdl, dev_tgt.device_id);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_get_app_ctrl_en(dev_tgt, app_id, is_enabled);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_get_app_ctrl_en(dev_tgt, app_id, is_enabled);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_get_app_ctrl_en(dev_tgt, app_id, is_enabled);
      break;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      goto done;
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

void pipe_mgr_pktgen_txn_commit(bf_dev_id_t dev) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return;
  // rmt_dev_cfg_t *cfg = &dev_info->dev_cfg;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_tof_pktgen_txn_commit(dev);
      return;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pktgen_txn_commit(dev);
      return;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pktgen_txn_commit(dev);
      return;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return;
  }
}
void pipe_mgr_pktgen_txn_abort(bf_dev_id_t dev) {
  uint32_t max_app_id;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return;
  max_app_id = pipe_mgr_pktgen_get_app_count(dev);
  // rmt_dev_cfg_t *cfg = &dev_info->dev_cfg;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_tof_pktgen_txn_abort(
          dev, max_app_id, dev_info->num_active_pipes);
      return;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pktgen_txn_abort(
          dev, max_app_id, dev_info->num_active_pipes);
      return;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pktgen_txn_abort(
          dev, max_app_id, dev_info->num_active_pipes);
      return;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d", __func__, __LINE__, dev);
      return;
  }
}

/* Update pkt buffer shadow memory */
static void pkt_buffer_shadow_mem_update(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         uint32_t offset,
                                         const uint8_t *buf,
                                         uint32_t size) {
  bool txn = pipe_mgr_sess_in_txn(shdl);
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) return;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mgr_tof_pkt_buffer_shadow_mem_update(
          dev_tgt, offset, buf, size, txn);
      return;
    case BF_DEV_FAMILY_TOFINO2:
      pipe_mgr_tof2_pkt_buffer_shadow_mem_update(
          dev_tgt, offset, buf, size, txn);
      return;
    case BF_DEV_FAMILY_TOFINO3:
      pipe_mgr_tof3_pkt_buffer_shadow_mem_update(
          dev_tgt.device_id, dev_tgt.dev_pipe_id, offset, buf, size, txn);
      return;
    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      return;
  }
  return;
}

static bool pktgen_pkt_buffer_size_offset_valid(bf_dev_target_t dev_tgt,
                                                uint32_t pktgen_byte_buf_offset,
                                                uint32_t size) {
  /* Must be 16B aligned. */
  if (pktgen_byte_buf_offset & 0xF) {
    LOG_ERROR(
        "%s:%d Invalid pktgen_byte_buf_offset 0x%x, offset must be 16B "
        "aligned, dev %d",
        __func__,
        __LINE__,
        pktgen_byte_buf_offset,
        dev_tgt.device_id);
    return false;
  }
  /* Max size is 16k. */
  if (size > PIPE_MGR_PKT_BUFFER_SIZE) {
    LOG_ERROR("%s:%d Invalid size, dev %d, size 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              size);
    return false;
  }
  if (pktgen_byte_buf_offset >= PIPE_MGR_PKT_BUFFER_SIZE) {
    LOG_ERROR("%s:%d Invalid pktgen_byte_buf_offset, dev %d, offset 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              pktgen_byte_buf_offset);
    return false;
  }
  if ((pktgen_byte_buf_offset + size) > PIPE_MGR_PKT_BUFFER_SIZE) {
    LOG_ERROR("%s:%d Invalid offset and size, dev %d, offset 0x%x, size 0x%x",
              __func__,
              __LINE__,
              dev_tgt.device_id,
              pktgen_byte_buf_offset,
              size);
    return false;
  }
  return true;
}

/* Write pkt buffer shadow memory to asic */
bf_status_t bf_pktgen_write_pkt_buffer(bf_session_hdl_t shdl,
                                       bf_dev_target_t dev_tgt,
                                       uint32_t pktgen_byte_buf_offset,
                                       uint32_t size,
                                       const uint8_t *buf) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_pkt_buffer_size_offset_valid(
          dev_tgt, pktgen_byte_buf_offset, size)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!buf) {
    LOG_ERROR("%s:%d Invalid buf pointer, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  /* Update shadow memory */
  pkt_buffer_shadow_mem_update(
      shdl, dev_tgt, pktgen_byte_buf_offset, buf, size);
  // bf_dev_pipe_t phy_pipe = 0;
  // pipe_mgr_map_pipe_id_log_to_phy(dev_info, dev_tgt.dev_pipe_id, &phy_pipe);
  int row = pktgen_byte_buf_offset / PIPE_MGR_PKT_BUFFER_WIDTH;
  int num_rows =
      (size + PIPE_MGR_PKT_BUFFER_WIDTH - 1) / PIPE_MGR_PKT_BUFFER_WIDTH;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_write_pkt_buffer(shdl, dev_tgt, row, num_rows);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_write_pkt_buffer(shdl, dev_tgt, row, num_rows);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_write_pkt_buffer(shdl, dev_tgt, row, num_rows);
      break;

    default:
      PIPE_MGR_DBGCHK(0);
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      break;
  }

done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_clear_pkt_buffer(bf_session_hdl_t shdl,
                                       bf_dev_target_t dev_tgt) {
  uint8_t *buf = PIPE_MGR_CALLOC(PIPE_MGR_PKT_BUFFER_SIZE, sizeof(uint8_t));
  if (!buf) return BF_NO_SYS_RESOURCES;
  bf_status_t sts = bf_pktgen_write_pkt_buffer(
      shdl, dev_tgt, 0, PIPE_MGR_PKT_BUFFER_SIZE, buf);
  if (buf) PIPE_MGR_FREE(buf);
  return sts;
}

bf_status_t bf_pktgen_pkt_buffer_get(bf_session_hdl_t shdl,
                                     bf_dev_target_t dev_tgt,
                                     uint32_t pktgen_byte_buf_offset,
                                     uint32_t size,
                                     uint8_t *buf) {
  bf_status_t sts = BF_SUCCESS;
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (!pktgen_pkt_buffer_size_offset_valid(
          dev_tgt, pktgen_byte_buf_offset, size)) {
    return BF_INVALID_ARG;
  }
  if (!buf) {
    LOG_ERROR("%s:%d Invalid buf pointer, dev %d",
              __func__,
              __LINE__,
              dev_tgt.device_id);
    return BF_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    return BF_INVALID_ARG;
  }

  PG_API_START(shdl, dev_tgt.device_id);
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pkt_buffer_shadow_mem_get(
          dev_tgt, pktgen_byte_buf_offset, buf, size);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pkt_buffer_shadow_mem_get(
          dev_tgt, pktgen_byte_buf_offset, buf, size);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pkt_buffer_shadow_mem_get(dev_tgt.device_id,
                                                    dev_tgt.dev_pipe_id,
                                                    pktgen_byte_buf_offset,
                                                    buf,
                                                    size);
      break;
    default:
      PIPE_MGR_DBGCHK(0);
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      break;
  }
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t pipe_mgr_pktgen_create_dma(bf_session_hdl_t shdl,
                                       rmt_dev_info_t *dev_info) {
  bf_dev_target_t dev_tgt = {dev_info->dev_id, 0};
  for (unsigned int i = 0; i < dev_info->num_active_pipes; ++i) {
    dev_tgt.dev_pipe_id = i;
    bf_status_t sts = pipe_mgr_pktgen_buffer_write_from_shadow(shdl, dev_tgt);
    if (BF_SUCCESS != sts) return sts;
  }
  return BF_SUCCESS;
}
bf_status_t pipe_mgr_pktgen_buffer_write_from_shadow(bf_session_hdl_t shdl,
                                                     bf_dev_target_t dev_tgt) {
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  bf_status_t sts;
  if (!dev_info) return BF_INVALID_ARG;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pkt_buffer_write_from_shadow(shdl, dev_tgt);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pkt_buffer_write_from_shadow(shdl, dev_tgt);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pkt_buffer_write_from_shadow(
          shdl, dev_tgt.device_id, dev_tgt.dev_pipe_id);
      break;




    default:
      sts = BF_UNEXPECTED;
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      break;
  }
  return sts;
}

bf_status_t bf_pktgen_get_batch_counter(bf_session_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        int app_id,
                                        uint64_t *batch_cnt) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  bf_dev_id_t dev = dev_tgt.device_id;
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!batch_cnt) {
    LOG_ERROR(
        "%s:%d Invalid batch_cnt pointer, dev %d", __func__, __LINE__, dev);
    sts = BF_INVALID_ARG;
    goto done;
  }

  int start_pipe, end_pipe;
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    start_pipe = 0;
    end_pipe = dev_info->num_active_pipes - 1;
  } else {
    start_pipe = end_pipe = dev_tgt.dev_pipe_id;
  }
  *batch_cnt = 0;
  for (int log_pipe = start_pipe; log_pipe <= end_pipe; ++log_pipe) {
    uint64_t pipe_cnt = 0;
    dev_tgt.dev_pipe_id = log_pipe;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        sts = pipe_mgr_tof_pktgen_reg_app_batch_ctr(dev_tgt, app_id, &pipe_cnt);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        sts =
            pipe_mgr_tof2_pktgen_reg_app_batch_ctr(dev_tgt, app_id, &pipe_cnt);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        sts = pipe_mgr_tof3_pktgen_reg_app_batch_ctr(
            dev, log_pipe, app_id, &pipe_cnt);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                  __func__,
                  __LINE__,
                  dev_tgt.device_id);
        sts = BF_UNEXPECTED;
        break;
    }
    if (BF_SUCCESS != sts) break;
    *batch_cnt += pipe_cnt;
  }

done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_get_pkt_counter(bf_session_hdl_t shdl,
                                      bf_dev_target_t dev_tgt,
                                      int app_id,
                                      uint64_t *pkt_cnt) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  bf_dev_id_t dev = dev_tgt.device_id;
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pkt_cnt) {
    LOG_ERROR("%s:%d Invalid pkt_cnt pointer, dev %d", __func__, __LINE__, dev);
    sts = BF_INVALID_ARG;
    goto done;
  }

  int start_pipe, end_pipe;
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    start_pipe = 0;
    end_pipe = dev_info->num_active_pipes - 1;
  } else {
    start_pipe = end_pipe = dev_tgt.dev_pipe_id;
  }
  *pkt_cnt = 0;
  for (int log_pipe = start_pipe; log_pipe <= end_pipe; ++log_pipe) {
    uint64_t pipe_cnt = 0;
    dev_tgt.dev_pipe_id = log_pipe;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        sts = pipe_mgr_tof_pktgen_reg_app_pkt_ctr(dev_tgt, app_id, &pipe_cnt);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        sts = pipe_mgr_tof2_pktgen_reg_app_pkt_ctr(dev_tgt, app_id, &pipe_cnt);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        sts = pipe_mgr_tof3_pktgen_reg_app_pkt_ctr(
            dev, log_pipe, app_id, &pipe_cnt);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                  __func__,
                  __LINE__,
                  dev_tgt.device_id);
        sts = BF_UNEXPECTED;
        break;
    }
    if (BF_SUCCESS != sts) break;
    *pkt_cnt += pipe_cnt;
  }

done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_get_trigger_counter(bf_session_hdl_t shdl,
                                          bf_dev_target_t dev_tgt,
                                          int app_id,
                                          uint64_t *trigger_cnt) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  bf_dev_id_t dev = dev_tgt.device_id;
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!trigger_cnt) {
    LOG_ERROR(
        "%s:%d Invalid trigger_cnt pointer, dev %d", __func__, __LINE__, dev);
    sts = BF_INVALID_ARG;
    goto done;
  }

  int start_pipe, end_pipe;
  if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
    start_pipe = 0;
    end_pipe = dev_info->num_active_pipes - 1;
  } else {
    start_pipe = end_pipe = dev_tgt.dev_pipe_id;
  }
  *trigger_cnt = 0;
  for (int log_pipe = start_pipe; log_pipe <= end_pipe; ++log_pipe) {
    uint64_t pipe_cnt = 0;
    dev_tgt.dev_pipe_id = log_pipe;
    switch (dev_info->dev_family) {
      case BF_DEV_FAMILY_TOFINO:
        sts = pipe_mgr_tof_pktgen_reg_app_trig_ctr(dev_tgt, app_id, &pipe_cnt);
        break;
      case BF_DEV_FAMILY_TOFINO2:
        sts = pipe_mgr_tof2_pktgen_reg_app_trig_ctr(dev_tgt, app_id, &pipe_cnt);
        break;
      case BF_DEV_FAMILY_TOFINO3:
        sts = pipe_mgr_tof3_pktgen_reg_app_trig_ctr(
            dev, log_pipe, app_id, &pipe_cnt);
        break;

      case BF_DEV_FAMILY_UNKNOWN:
        LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                  __func__,
                  __LINE__,
                  dev_tgt.device_id);
        sts = BF_UNEXPECTED;
        break;
    }
    if (BF_SUCCESS != sts) break;
    *trigger_cnt += pipe_cnt;
  }

done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_set_batch_counter(bf_session_hdl_t shdl,
                                        bf_dev_target_t dev_tgt,
                                        int app_id,
                                        uint64_t count) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  bf_dev_id_t dev = dev_tgt.device_id;
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_reg_app_batch_ctr_set(
          shdl, dev_tgt, app_id, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_reg_app_batch_ctr_set(
          shdl, dev_tgt, app_id, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_reg_app_batch_ctr_set(
          shdl, dev, dev_tgt.dev_pipe_id, app_id, count);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      break;
  }

done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_set_pkt_counter(bf_session_hdl_t shdl,
                                      bf_dev_target_t dev_tgt,
                                      int app_id,
                                      uint64_t count) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  bf_dev_id_t dev = dev_tgt.device_id;
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts =
          pipe_mgr_tof_pktgen_reg_app_pkt_ctr_set(shdl, dev_tgt, app_id, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_reg_app_pkt_ctr_set(
          shdl, dev_tgt, app_id, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_reg_app_pkt_ctr_set(
          shdl, dev, dev_tgt.dev_pipe_id, app_id, count);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      break;
  }

done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_set_trigger_counter(bf_session_hdl_t shdl,
                                          bf_dev_target_t dev_tgt,
                                          int app_id,
                                          uint64_t count) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  bf_dev_id_t dev = dev_tgt.device_id;
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_reg_app_trig_ctr_set(
          shdl, dev_tgt, app_id, count);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_reg_app_trig_ctr_set(
          shdl, dev_tgt, app_id, count);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_reg_app_trig_ctr_set(
          shdl, dev, dev_tgt.dev_pipe_id, app_id, count);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      break;
  }

done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_cfg_app(bf_session_hdl_t shdl,
                              bf_dev_target_t dev_tgt,
                              int app_id,
                              bf_pktgen_app_cfg_t *cfg) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_cfg_app_conf_check(
          dev_info, dev_tgt, app_id, cfg);
      if (sts != BF_SUCCESS) goto done;
      sts = pipe_mgr_tof_pktgen_cfg_app(shdl, dev_tgt, app_id, cfg);
      break;

    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_cfg_app_conf_check(dev_tgt, app_id, cfg);
      if (sts != BF_SUCCESS) goto done;
      sts = pipe_mgr_tof2_pktgen_cfg_app(shdl, dev_tgt, app_id, cfg);
      break;

    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_cfg_app_conf_check(
          dev_info, dev_tgt, app_id, cfg);
      if (sts != BF_SUCCESS) goto done;
      sts = pipe_mgr_tof3_pktgen_cfg_app(shdl, dev_tgt, app_id, cfg);
      break;

    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      goto done;
  }

done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_cfg_app_get(bf_session_hdl_t shdl,
                                  bf_dev_target_t dev_tgt,
                                  int app_id,
                                  bf_pktgen_app_cfg_t *cfg) {
  bf_status_t sts = BF_SUCCESS;
  if (cfg == NULL) return BF_INVALID_ARG;
  PG_API_START(shdl, dev_tgt.device_id);
  if (!pipe_mgr_valid_session(&shdl, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (!pktgen_valid_dev_tgt(dev_tgt, __func__, __LINE__)) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  if (illegal_app_id(dev_tgt.device_id, app_id)) {
    LOG_ERROR("%s:%d Invalid application id %d, dev %d",
              __func__,
              __LINE__,
              app_id,
              dev_tgt.device_id);
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    sts = BF_INVALID_ARG;
    goto done;
  }

  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = pipe_mgr_tof_pktgen_cfg_app_get(dev_tgt, app_id, cfg);
      break;

    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_cfg_app_get(dev_tgt, app_id, cfg);
      break;

    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_cfg_app_get(dev_tgt, app_id, cfg);
      break;

    default:
      LOG_ERROR("%s:%d Invalid dev_family, dev %d",
                __func__,
                __LINE__,
                dev_tgt.device_id);
      sts = BF_UNEXPECTED;
      goto done;
  }

done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_cfg_port_down_mask(bf_session_hdl_t shdl,
                                         bf_dev_target_t dev_tgt,
                                         uint32_t port_mask_sel,
                                         struct bf_tof2_port_down_sel *mask) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);
  if ((port_mask_sel > 1) || (mask == NULL)) {
    LOG_ERROR(
        "%s:%d Invalid mask_sel or invalid mask, mask_sel has to be 0 or 1, "
        "dev %d, port_mask_sel %d",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        port_mask_sel);
    sts = BF_INVALID_ARG;
    goto done;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    sts = BF_INVALID_ARG;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      sts = BF_INVALID_ARG;
      LOG_ERROR(
          "%s:%d Tofino does not support port_down mask configuration, dev %d",
          __func__,
          __LINE__,
          dev_tgt.device_id);
      goto done;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_cfg_port_down_mask(
          shdl, dev_tgt, port_mask_sel, mask);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_cfg_port_down_mask(
          shdl, dev_tgt, port_mask_sel, mask);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      sts = PIPE_UNEXPECTED;
      PIPE_MGR_DBGCHK(0);
      break;
  }

done:
  PG_API_END(shdl, sts, false);
  return sts;
}

bf_status_t bf_pktgen_port_down_mask_get(bf_dev_target_t dev_tgt,
                                         uint32_t port_mask_sel,
                                         struct bf_tof2_port_down_sel *mask) {
  if ((port_mask_sel > 1) || (mask == NULL)) {
    LOG_ERROR(
        "%s:%d Invalid mask_sel or invalid mask, mask_sel has to be 0 or 1, "
        "dev %d, port_mask_sel %d",
        __func__,
        __LINE__,
        dev_tgt.device_id,
        port_mask_sel);
    return BF_INVALID_ARG;
  }
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    return BF_INVALID_ARG;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      LOG_ERROR(
          "%s:%d Tofino does not support port_down mask configuration, dev %d",
          __func__,
          __LINE__,
          dev_tgt.device_id);
      return BF_INVALID_ARG;
    case BF_DEV_FAMILY_TOFINO2:
      return pipe_mgr_tof2_pktgen_cfg_port_down_mask_get(
          dev_tgt, port_mask_sel, mask);
    case BF_DEV_FAMILY_TOFINO3:
      return pipe_mgr_tof3_pktgen_cfg_port_down_mask_get(
          dev_tgt, port_mask_sel, mask);

    case BF_DEV_FAMILY_UNKNOWN:
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  return BF_INVALID_ARG;
}

bf_status_t bf_pktgen_port_down_replay_mode_set(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t mode) {
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    sts = BF_INVALID_ARG;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (mode != BF_PKTGEN_PORT_DOWN_REPLAY_NONE) {
        sts = BF_INVALID_ARG;
        LOG_ERROR(
            "%s:%d Tofino only supports %s, requested %s, dev %d",
            __func__,
            __LINE__,
            bf_pktgen_port_down_mode_string(BF_PKTGEN_PORT_DOWN_REPLAY_NONE),
            bf_pktgen_port_down_mode_string(mode),
            dev_tgt.device_id);
      } else {
        sts = PIPE_SUCCESS;
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_cfg_port_down_replay(shdl, dev_tgt, mode);
      if (PIPE_SUCCESS == sts) {
        if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
          for (unsigned i = 0; i < dev_info->num_active_pipes; ++i)
            pipe_mgr_pktgen_ctx(dev_tgt.device_id)
                ->u.tof2_ctx->port_down_mode[i] = mode;
        } else {
          pipe_mgr_pktgen_ctx(dev_tgt.device_id)
              ->u.tof2_ctx->port_down_mode[dev_tgt.dev_pipe_id] = mode;
        }
      }
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_cfg_port_down_replay(shdl, dev_tgt, mode);
      if (PIPE_SUCCESS == sts) {
        if (BF_DEV_PIPE_ALL == dev_tgt.dev_pipe_id) {
          for (unsigned i = 0; i < dev_info->num_active_pipes; ++i)
            pipe_mgr_pktgen_ctx(dev_tgt.device_id)
                ->u.tof3_ctx->port_down_mode[i] = mode;
        } else {
          pipe_mgr_pktgen_ctx(dev_tgt.device_id)
              ->u.tof3_ctx->port_down_mode[dev_tgt.dev_pipe_id] = mode;
        }
      }
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      sts = PIPE_UNEXPECTED;
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

bf_status_t bf_pktgen_port_down_replay_mode_get(
    bf_session_hdl_t shdl,
    bf_dev_target_t dev_tgt,
    bf_pktgen_port_down_mode_t *mode) {
  if (!mode) return BF_INVALID_ARG;
  bf_status_t sts = BF_SUCCESS;
  PG_API_START(shdl, dev_tgt.device_id);

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_tgt.device_id);
  if (!dev_info) {
    PIPE_MGR_DBGCHK(dev_info);
    sts = BF_INVALID_ARG;
    goto done;
  }
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      *mode = BF_PKTGEN_PORT_DOWN_REPLAY_NONE;
      sts = PIPE_SUCCESS;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      sts = pipe_mgr_tof2_pktgen_get_port_down_replay(dev_info, dev_tgt, mode);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      sts = pipe_mgr_tof3_pktgen_get_port_down_replay(
          shdl, dev_info, dev_tgt, mode);
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      sts = PIPE_UNEXPECTED;
      PIPE_MGR_DBGCHK(0);
      break;
  }
done:
  PG_API_END(shdl, sts, true);
  return sts;
}

uint32_t bf_pktgen_get_app_count(bf_dev_id_t dev_id) {
  return pipe_mgr_pktgen_get_app_count(dev_id);
}

bf_dev_port_t bf_pktgen_port_get(bf_dev_id_t dev_id) {
  bf_dev_type_t dev_type = bf_drv_get_dev_type(dev_id);
  return (bf_is_dev_type_family_tofino(dev_type)) ? 64 : 0;
}

bf_status_t bf_pktgen_get_next_port(bf_dev_id_t dev_id, bf_dev_port_t *port) {
  if (!port) return BF_INVALID_ARG;
  const bf_dev_port_t max_port = bf_pktgen_max_port_get(dev_id);
  if (*port > max_port) return BF_INVALID_ARG;
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  bf_status_t sts = pipe_mgr_api_enter(shdl);
  if (sts != PIPE_SUCCESS) return sts;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Invalid device id %d", __func__, __LINE__, dev_id);
    pipe_mgr_api_exit(shdl);
    return BF_INVALID_ARG;
  }
  if (!dev_info->dev_cfg.dev_port_validate(*port)) {
    LOG_ERROR("%s:%d Invalid port, dev %d, port 0x%x",
              __func__,
              __LINE__,
              dev_id,
              *port);
    pipe_mgr_api_exit(shdl);
    return BF_INVALID_ARG;
  }
  int step = 1;
  bf_dev_port_t max_local_port = 0;
  switch (dev_info->dev_family) {
    case BF_DEV_FAMILY_TOFINO:  // step is 1 for TF1 and TF2
      max_local_port = 71;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      max_local_port = 7;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      step = 2;
      break;

    case BF_DEV_FAMILY_UNKNOWN:
      pipe_mgr_api_exit(shdl);
      PIPE_MGR_DBGCHK(0);
      return PIPE_UNEXPECTED;
  }
  bf_dev_port_t next_port = *port + step;
  bf_dev_port_t local_port = next_port % 128;
  if (local_port > max_local_port) {
    // increment to lowest pktgen port on next pipe
    next_port += 128 + bf_pktgen_port_get(dev_id) - local_port;
  }
  *port = next_port;
  pipe_mgr_api_exit(shdl);
  return (*port > max_port) ? PIPE_OBJ_NOT_FOUND : PIPE_SUCCESS;
}

bf_dev_port_t bf_pktgen_max_port_get(bf_dev_id_t dev_id) {
  pipe_sess_hdl_t shdl = pipe_mgr_get_int_sess_hdl();
  bf_status_t sts = pipe_mgr_api_enter(shdl);
  if (sts != PIPE_SUCCESS) return sts;
  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
  if (!dev_info) {
    LOG_ERROR("%s:%d Invalid device id %d", __func__, __LINE__, dev_id);
    pipe_mgr_api_exit(shdl);
    return PIPE_INVALID_ARG;
  }
  unsigned int max_pipe = dev_info->num_active_pipes - 1;
  bf_dev_port_t max_local_port =
      (dev_info->dev_family == BF_DEV_FAMILY_TOFINO) ? 71 : 7;
  bf_dev_port_t max_port =
      dev_info->dev_cfg.make_dev_port(max_pipe, max_local_port);
  pipe_mgr_api_exit(shdl);
  return max_port;
}

void pd_cfg_to_bf_pktgen_app_cfg(bf_pktgen_app_cfg_t *dest,
                                 p4_pd_pktgen_app_cfg *const src) {
  if (!dest || !src || dest == (bf_pktgen_app_cfg_t *)src) {
    return;
  }

  dest->trigger_type = (bf_pktgen_trigger_type_e)src->trigger_type;
  dest->batch_count = src->batch_count;
  dest->packets_per_batch = src->packets_per_batch;
  if (BF_PKTGEN_TRIGGER_RECIRC_PATTERN == dest->trigger_type) {
    dest->u.pattern.value = src->pattern_value;
    dest->u.pattern.mask = src->pattern_mask;
  } else {
    dest->u.timer_nanosec = src->timer_nanosec;
  }
  dest->ibg = src->ibg;
  dest->ibg_jitter = src->ibg_jitter;
  dest->ipg = src->ipg;
  dest->ipg_jitter = src->ipg_jitter;
  dest->pipe_local_source_port = src->source_port;
  dest->increment_source_port = src->increment_source_port;
  dest->pkt_buffer_offset = src->pkt_buffer_offset;
  dest->length = src->length;
}

void pd_cfg_to_bf_pktgen_app_cfg_tof2(bf_pktgen_app_cfg_t *dest,
                                      p4_pd_pktgen_app_cfg_tof2 *const src) {
  if (!dest || !src || dest == (bf_pktgen_app_cfg_t *)src) {
    return;
  }

  dest->trigger_type = (bf_pktgen_trigger_type_e)src->trigger_type;
  dest->batch_count = src->batch_count;
  dest->packets_per_batch = src->packets_per_batch;
  switch (dest->trigger_type) {
    case BF_PKTGEN_TRIGGER_RECIRC_PATTERN:
    case BF_PKTGEN_TRIGGER_DPRSR: {
      const int val_size =
          sizeof(src->pattern_value_long) / sizeof(src->pattern_value_long[0]);
      const int mask_size =
          sizeof(src->pattern_mask_long) / sizeof(src->pattern_mask_long[0]);
      memcpy(dest->u.pattern_tof2.value, src->pattern_value_long, val_size);
      memcpy(dest->u.pattern_tof2.mask, src->pattern_mask_long, mask_size);
      break;
    }
    case BF_PKTGEN_TRIGGER_PFC: {
      const int pfc_size = sizeof(src->pfc_hdr) / sizeof(src->pfc_hdr[0]);
      memcpy(dest->u.pfc_cfg.pfc_hdr, src->pfc_hdr, pfc_size);
      dest->u.pfc_cfg.cfg_timer_en = src->pfc_timer_en;
      dest->u.pfc_cfg.cfg_timer = src->pfc_timer;
      dest->u.pfc_cfg.pfc_max_msgs = src->pfc_max_msgs;
      break;
    }
    case BF_PKTGEN_TRIGGER_PORT_DOWN:
      dest->u.port_mask_sel_tof2 = (uint32_t)src->port_mask_sel;
      break;
    default:
      dest->u.timer_nanosec = src->timer_nanosec;
      break;
  }
  dest->ibg = src->ibg;
  dest->ibg_jitter = src->ibg_jitter;
  dest->ipg = src->ipg;
  dest->ipg_jitter = src->ipg_jitter;
  dest->pipe_local_source_port = src->source_port;
  dest->increment_source_port = src->increment_source_port;
  dest->pkt_buffer_offset = src->pkt_buffer_offset;
  dest->length = src->length;
  dest->tof2.offset_len_from_recir_pkt = src->offset_len_from_recir_pkt;
  dest->tof2.source_port_wrap_max = src->source_port_wrap_max;
  dest->tof2.assigned_chnl_id = src->assigned_chnl_id;
}

void bf_cfg_to_pd_pktgen_app_cfg(p4_pd_pktgen_app_cfg *dest,
                                 bf_pktgen_app_cfg_t *const src) {
  if (!dest || !src || dest == (p4_pd_pktgen_app_cfg *)src) {
    return;
  }

  dest->trigger_type = (p4_pd_pktgen_trigger_type_e)src->trigger_type;
  dest->batch_count = src->batch_count;
  dest->packets_per_batch = src->packets_per_batch;
  if (BF_PKTGEN_TRIGGER_RECIRC_PATTERN == src->trigger_type) {
    dest->pattern_value = src->u.pattern.value;
    dest->pattern_mask = src->u.pattern.mask;
  } else {
    dest->timer_nanosec = src->u.timer_nanosec;
  }
  dest->ibg = src->ibg;
  dest->ibg_jitter = src->ibg_jitter;
  dest->ipg = src->ipg;
  dest->ipg_jitter = src->ipg_jitter;
  dest->source_port = src->pipe_local_source_port;
  dest->increment_source_port = src->increment_source_port;
  dest->pkt_buffer_offset = src->pkt_buffer_offset;
  dest->length = src->length;
}

void bf_cfg_to_pd_pktgen_app_cfg_tof2(p4_pd_pktgen_app_cfg_tof2 *dest,
                                      bf_pktgen_app_cfg_t *const src) {
  if (!dest || !src || dest == (p4_pd_pktgen_app_cfg_tof2 *)src) {
    return;
  }

  dest->trigger_type = (p4_pd_pktgen_trigger_type_e)src->trigger_type;
  dest->batch_count = src->batch_count;
  dest->packets_per_batch = src->packets_per_batch;
  switch (src->trigger_type) {
    case BF_PKTGEN_TRIGGER_RECIRC_PATTERN:
    case BF_PKTGEN_TRIGGER_DPRSR: {
      const int val_size = sizeof(dest->pattern_value_long) /
                           sizeof(dest->pattern_value_long[0]);
      const int mask_size =
          sizeof(dest->pattern_mask_long) / sizeof(dest->pattern_mask_long[0]);
      memcpy(dest->pattern_value_long, src->u.pattern_tof2.value, val_size);
      memcpy(dest->pattern_mask_long, src->u.pattern_tof2.mask, mask_size);
      break;
    }
    case BF_PKTGEN_TRIGGER_PFC: {
      const int pfc_size = sizeof(dest->pfc_hdr) / sizeof(dest->pfc_hdr[0]);
      memcpy(dest->pfc_hdr, src->u.pfc_cfg.pfc_hdr, pfc_size);
      dest->pfc_timer_en = src->u.pfc_cfg.cfg_timer_en;
      dest->pfc_timer = src->u.pfc_cfg.cfg_timer;
      dest->pfc_max_msgs = src->u.pfc_cfg.pfc_max_msgs;
      break;
    }
    case BF_PKTGEN_TRIGGER_PORT_DOWN:
      dest->port_mask_sel = (uint8_t)src->u.port_mask_sel_tof2;
      break;
    default:
      dest->timer_nanosec = src->u.timer_nanosec;
      break;
  }
  dest->ibg = src->ibg;
  dest->ibg_jitter = src->ibg_jitter;
  dest->ipg = src->ipg;
  dest->ipg_jitter = src->ipg_jitter;
  dest->source_port = src->pipe_local_source_port;
  dest->increment_source_port = src->increment_source_port;
  dest->pkt_buffer_offset = src->pkt_buffer_offset;
  dest->length = src->length;
  dest->offset_len_from_recir_pkt = src->tof2.offset_len_from_recir_pkt;
  dest->source_port_wrap_max = src->tof2.source_port_wrap_max;
  dest->assigned_chnl_id = src->tof2.assigned_chnl_id;
}
