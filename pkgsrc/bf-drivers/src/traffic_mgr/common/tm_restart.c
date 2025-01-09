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


#include <string.h>
#include <dvm/bf_drv_intf.h>
#include "tm_ctx.h"
#include "tm_init.h"
#include "tm_hw_access.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <traffic_mgr/tm_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/bf_dev_if.h>

/*
 *    This file has code to restore TM configuration by fetching from
 *    TM device.  TM config restoration is done when driver is
 *    restarted.
 *
 *    TM config restoration can fail when
 *      - Restoration was triggered when TM device was not fully
 *        initialized (prior to driver restart, default config is
 *        incomplete)
 *
 *      - During config restoration, driver could not satisfy complete
 *        config rebuild operation.
 */

/* Allocated but unused/unmapped PPGs cannot be restored from HW config */

// post restart, already allocated but unused PPGs can be allocated
// to app again if bf_tm_alloc_ppg() is called. This can result in duplicate
// PPG allocation resulting in unpredictable behaviour.
//
// Two ways to fix the problem.
// 1. We can take app help to filter PPG handles. App can filter
//    duplicate PPGs handles.  Any time PPG handle is allocated,
//    if such a handle was already allocated, app should purge
//    old PPG handle. Note if PPG handle was allocated and put to use
//    (by mapping traffic class to ppg) prior to restart/crash, then
//    there will not be duplicate handle.
// 2. At the end of restart recovery, App can replay all PPG handles
//    bf_tm_ppg_replay_handles(). This can help to establish unused
//    but allocated PPGs.

static bf_status_t bf_tm_restore_ppg_cfg(bf_dev_id_t dev) {
  int j, i, k, lport;
  bf_tm_ppg_t ppg, *_ppg, *restore_ppg;
  bf_tm_port_t *_p;
  bf_dev_port_t port;
  bf_dev_pipe_t pipe = 0;
  uint32_t num_pipes;
  uint8_t ppg_icos_mask;
  bf_tm_status_t rc = BF_TM_EOK;

  LOG_TRACE("%s:", __func__);

  rc = bf_tm_restore_wac_offset_profile(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore wac offset profile table cfg");
    return (rc);
  }
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    memset(&ppg, 0, sizeof(bf_tm_ppg_t));
    if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, j, &pipe)) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. Device = %d Logical "
          "pipe = %d",
          dev,
          j);
    }
    ppg.p_pipe = pipe;
    ppg.d_pipe = pipe % 4;
    ppg.l_pipe = j;
    _ppg = BF_TM_PPG_PTR(g_tm_restore_ctx[dev], j /*logical pipe*/, 0);
    BF_TM_PPG_CHECK(_ppg, 0, j, dev);

    for (lport = 0; lport < g_tm_restore_ctx[dev]->tm_cfg.ports_per_pipe;
         lport++) {
      ppg.port = lport;
      ppg_icos_mask = 0;
      for (i = 0; i < BF_TM_MAX_COS_LEVELS; i++) {
        ppg.ppg_cfg.icos_mask = (1 << i);
        if (ppg.ppg_cfg.icos_mask & ppg_icos_mask) {
          // previously restored ppgs have already
          // been mapped to the cos (1 << i)
          continue;
        }
        rc = bf_tm_ppg_get_allocation(dev, &ppg);
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG cfg");
          return (rc);
        }
        if (ppg.is_default_ppg) {
          ppg.ppg = lport + g_tm_restore_ctx[dev]->tm_cfg.pfc_ppg_per_pipe;
          restore_ppg = _ppg + ppg.ppg;
          restore_ppg->is_default_ppg = true;
        } else {
          // Non-Default/PFC ppg allocated to port, icos.
          restore_ppg = _ppg + ppg.ppg;
          restore_ppg->is_default_ppg = false;
        }
        bf_sys_assert((restore_ppg)->p_pipe == ppg.p_pipe);
        bf_sys_assert((restore_ppg)->l_pipe == ppg.l_pipe);
        bf_sys_assert((restore_ppg)->ppg == ppg.ppg);
        // This can be set only by replaying
        // TM PPG allocation API
        // restore_ppg->in_use = true;
        restore_ppg->port = lport;
        restore_ppg->ppg_cfg.app_poolid = ppg.ppg_cfg.app_poolid;

        rc = bf_tm_ppg_get_icos_mask(dev,
                                     restore_ppg,
                                     &(restore_ppg->ppg_cfg.icos_mask),
                                     &(restore_ppg->ppg_cfg.icos_mask));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG icos mask cfg");
          return (rc);
        }

        ppg_icos_mask |= restore_ppg->ppg_cfg.icos_mask;

        rc = bf_tm_ppg_get_pfc_treatment(dev,
                                         restore_ppg,
                                         &(restore_ppg->ppg_cfg.is_pfc),
                                         &(restore_ppg->ppg_cfg.is_pfc));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG pfc cfg");
          return (rc);
        }

        rc = bf_tm_ppg_get_min_limit(dev,
                                     restore_ppg,
                                     &(restore_ppg->thresholds.min_limit),
                                     &(restore_ppg->thresholds.min_limit));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG gmin limit cfg");
          return (rc);
        }
        if (!restore_ppg->is_default_ppg) {
          rc = bf_tm_ppg_get_skid_limit(dev,
                                        restore_ppg,
                                        &(restore_ppg->thresholds.skid_limit),
                                        &(restore_ppg->thresholds.skid_limit));
          if (rc != BF_SUCCESS) {
            LOG_ERROR("Unable to restore PPG skid limit cfg");
            return (rc);
          }
        }

        /* restore is_dynamic before app_limit */
        rc = bf_tm_ppg_get_is_dynamic(dev,
                                      restore_ppg,
                                      &(restore_ppg->ppg_cfg.is_dynamic),
                                      &(restore_ppg->ppg_cfg.is_dynamic));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG dynamic usage cfg");
          return (rc);
        }

        rc = bf_tm_ppg_get_app_limit(dev,
                                     restore_ppg,
                                     &(restore_ppg->thresholds.app_limit),
                                     &(restore_ppg->thresholds.app_limit));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG app limit cfg");
          return (rc);
        }
        rc =
            bf_tm_ppg_get_ppg_hyst_index(dev,
                                         restore_ppg,
                                         &(restore_ppg->thresholds.hyst_index),
                                         &(restore_ppg->thresholds.hyst_index));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG hysteresis index");
          return (rc);
        }
        rc = bf_tm_ppg_get_ppg_hyst(dev,
                                    restore_ppg,
                                    &(restore_ppg->thresholds.ppg_hyst),
                                    &(restore_ppg->thresholds.ppg_hyst));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore PPG hysteresis cfg");
          return (rc);
        }

        if (restore_ppg->ppg_cfg.is_dynamic) {
          rc = bf_tm_ppg_get_baf(dev,
                                 restore_ppg,
                                 &(restore_ppg->ppg_cfg.baf),
                                 &(restore_ppg->ppg_cfg.baf));
          if (rc != BF_SUCCESS) {
            LOG_ERROR("Unable to restore PPG baf cfg");
            return (rc);
          }
        }

        port = MAKE_DEV_PORT(j, lport);
        _p = BF_TM_PORT_PTR(g_tm_restore_ctx[dev], port);
        //_p->ppg_count += 1; ppg count can be set only by replaying TM ppg
        // allocation API.
        for (k = 0; k < BF_TM_MAX_COS_LEVELS; k++) {
          if ((1 << k) & restore_ppg->ppg_cfg.icos_mask) {
            // !!!! Config restore corner case. !!!!

            // If application allocated x ppgs for a port, but
            // only subset of x are in use / mapped to icos traffic,
            // then it is not possible to restore unused PPGs
            // since hardware has no way to store unmapped ppgs.
            // _p->ppg_list[k] = restore_ppg; // ppg_list gets updated during
            // ppg_allocate API for hitless HA
            _p->ppgs[k] = restore_ppg;
          }
        }

        // In Hitless Warm init cache counters only need to be applied for
        // Default PPG. For Non-Default PPG p4_pd_tm_allocate_ppg() is
        // called by the application stack later.
        if (ppg.is_default_ppg) {
          bf_tm_ppg_hdl ppg_hndl = ppg.ppg;
          ppg_hndl |= (j << 12);
          ppg_hndl |= (lport << 16);

          rc = bf_tm_ppg_set_cache_counters(dev, ppg_hndl);
          if (rc != BF_SUCCESS) {
            LOG_ERROR(
                "PPG handle 0x%x port %d Unable to allocate cache counters",
                ppg_hndl,
                port);
            return rc;
          }
        }
      }
    }
  }
  return (rc);
}

static bf_status_t bf_tm_restore_port_cfg(bf_dev_id_t dev) {
  bf_tm_port_t *_p;
  int j, lport;
  bf_dev_port_t port;
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t num_pipes;

  // Following port configs are restored when port is added.
  // offline, speed, ct_enabled. These will not be
  // restored here. Also, HW doesn't store all/some of these cfg.

  LOG_TRACE("%s:", __func__);
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_restore_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_restore_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      _p = BF_TM_PORT_PTR(g_tm_restore_ctx[dev], port);

      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to port descriptor for pipe %d, port %d", j, lport);
        return (rc);
      }
      bf_tm_q_get_port_q_profile(
          dev, port, _p->p_pipe, (uint32_t *)&(_p->qid_profile));
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "Unable to restore queue profile for pipe %d, port %d", j, lport);
        return (rc);
      }
      rc = bf_tm_port_get_wac_drop_limit(
          dev, _p, &(_p->wac_drop_limit), &(_p->wac_drop_limit));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore ingress port drop limit pipe %d, port %d",
                  j,
                  lport);
        return (rc);
      }
      rc = bf_tm_port_get_qac_drop_limit(
          dev, _p, &(_p->qac_drop_limit), &(_p->qac_drop_limit));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore egress port drop limit pipe %d, port %d",
                  j,
                  lport);
        return (rc);
      }
      rc = bf_tm_port_get_wac_hyst(
          dev, _p, &(_p->wac_resume_limit), &(_p->wac_resume_limit));
      if (rc == BF_NOT_SUPPORTED) {
        rc = BF_SUCCESS;
      }
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "Unable to restore  ingress port resume limit pipe %d, port %d",
            j,
            lport);
        return (rc);
      }
      rc = bf_tm_port_get_qac_hyst(
          dev, _p, &(_p->qac_resume_limit), &(_p->qac_resume_limit));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore egress port resume limit pipe %d, port %d",
                  j,
                  lport);
        return (rc);
      }
      rc = bf_tm_port_get_wac_hyst_index(
          dev, _p, &(_p->wac_hyst_index), &(_p->wac_hyst_index));
      if (rc == BF_NOT_SUPPORTED) {
        rc = BF_SUCCESS;
      }
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port WAC hysteresis index");
        return (rc);
      }

      rc = bf_tm_port_get_qac_hyst_index(
          dev, _p, &(_p->qac_hyst_index), &(_p->qac_hyst_index));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port QAC hysteresis index");
        return (rc);
      }

      rc = bf_tm_port_get_flowcontrol_mode(
          dev, _p, &(_p->fc_type), &(_p->fc_type));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port flow control mode");
        return (rc);
      }
      rc = bf_tm_port_get_flowcontrol_rx(
          dev, _p, &(_p->fc_rx_type), &(_p->fc_rx_type));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port rx flow control mode");
        return (rc);
      }
      rc = bf_tm_port_get_pfc_cos_mask(
          dev, _p, &(_p->icos_to_cos_mask), &(_p->icos_to_cos_mask));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port pfc cos mapping");
        return (rc);
      }
      rc = bf_tm_port_get_uc_cut_through_limit(
          dev, _p, &(_p->uc_cut_through_limit), &(_p->uc_cut_through_limit));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port uc cut through limit");
        return (rc);
      }
      rc = bf_tm_port_get_cut_through_enable_status(
          dev, _p, &(_p->ct_enabled), &(_p->ct_enabled));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port cut-through enable status");
        return (rc);
      }

      rc = bf_tm_sch_get_port_max_rate(dev,
                                       _p,
                                       &(_p->pps),
                                       &(_p->burst_size),
                                       &(_p->burst_size),
                                       &(_p->port_rate),
                                       &(_p->port_rate));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port max rate");
        return (rc);
      }

      rc = bf_tm_sch_get_port_max_rate_enable_status(
          dev, _p, &(_p->max_rate_enabled), &(_p->max_rate_enabled));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port shaping state");
        return (rc);
      }

      rc = bf_tm_sch_get_port_sched(
          dev, _p, &(_p->sch_enabled), &(_p->sch_enabled));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore port scheduling state");
        return (rc);
      }

      // If existing ports are added after cfg restore, port_rate,
      // uc_cut_through_limit  will be reset to default. Resolve this later. If
      // ports are added before cfg is restored, then port setting will reset to
      // default wiping config on chip. Need a mechanism to distinguish ports
      // that were added prior to HA/restart to be added again with a flag
      // indicating port-re-addition and not new port addition.
      // check bf_tm_add_new_port()

      _p->cfg_restored = true;
    }
  }
  return (rc);
}

static bf_status_t bf_tm_restore_q_profile_cfg(bf_dev_id_t dev) {
  // Fetch qid mapping table from HW and build 'n' profiles that asic supports.
  bf_tm_status_t rc = BF_TM_EOK;
  uint32_t j, lport, i;
  bf_dev_port_t port;
  bf_dev_pipe_t pipe = 0;
  uint32_t num_pipes;
  bf_tm_port_t *_p;
  int port_ch, q_count, base_q, queue;
  int min_q, max_q;

  // Restore all queue profile mappings from HW
  // On Tofino, we have global queue profiles which are shared across the ports.
  // On Tofino-2 and above, each port can have it's own queue mapping, so the
  // number of queue profiles is equal to the number of ports
  rc = bf_tm_q_get_profiles_mapping(dev, g_tm_restore_ctx[dev]->q_profile);
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "%s: Unable to fetch queue profile configuration for dev %d, rc %s "
        "(%d)",
        __func__,
        dev,
        bf_err_str(rc),
        rc);
    return (rc);
  }

  LOG_TRACE("%s:", __func__);
  lld_sku_get_num_active_pipes(dev, &num_pipes);

  for (j = 0; j < num_pipes; j++) {
    // Get physical pipe
    if (lld_sku_map_pipe_id_to_phy_pipe_id(dev, j, &pipe) != LLD_OK) {
      LOG_ERROR(
          "Unable to map logical pipe to physical pipe id. Device = %d Logical "
          "pipe = %d",
          dev,
          j);
      return BF_INTERNAL_ERROR;
    }

    for (lport = 0; lport < g_tm_restore_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_restore_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      _p = BF_TM_PORT_PTR(g_tm_restore_ctx[dev], port);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to port descriptor for pipe %d, port %d", j, lport);
        return (rc);
      }
      bf_tm_q_get_port_q_profile(
          dev, port, _p->p_pipe, (uint32_t *)&(_p->qid_profile));
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "Unable to restore queue profile for pipe %d, port %d", j, lport);
        return (rc);
      }
      if (!((g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->in_use)) {
        port_ch = DEV_PORT_TO_LOCAL_PORT(port) %
                  g_tm_restore_ctx[dev]->tm_cfg.ports_per_pg;
        (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->ch_in_pg =
            port_ch;

        /*
         * Restore the base queue -
         *   - For port channel = 0, base_q is always 0
         *   - For rest of the port channels, base_q is previous port's
         *     base_q + previous port's queue count
         *
         */
        if (port_ch == 0) {
          base_q = 0;
        } else {
          rc = bf_tm_q_get_base_queue(dev, port, port_ch, &base_q);
          if (BF_TM_IS_NOTOK(rc)) {
            LOG_ERROR(
                "Unable to get base queue for dev %d port %d "
                "port-ch %d, rc %s (%d)",
                dev,
                port,
                port_ch,
                bf_err_str(rc),
                rc);
            return (rc);
          }
        }
        (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->base_q = base_q;

        if ((g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->base_q) {
          // q_mapping restored from HW has base_q added. Subtract that.
          for (i = 0; i < g_tm_restore_ctx[dev]->tm_cfg.q_per_pg; i++) {
            (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)
                ->q_mapping[i] -=
                (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->base_q;
          }
        }

        /*
         * Restore queue count - this is bit tricky
         * Since we support flexible queue mapping, best is to identify the
         * min queue number and max queue number and calculate the queue
         * count (this is under the assumption that the queue mapping is
         * done for the first and last queue at least.
         *
         */
        min_q = max_q =
            (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->q_mapping[0];
        for (i = 1; i < g_tm_restore_ctx[dev]->tm_cfg.q_per_pg; i++) {
          queue = (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)
                      ->q_mapping[i];

          if (queue < min_q) {
            min_q = queue;
          }

          if (queue > max_q) {
            max_q = queue;
          }
        }

        q_count = max_q - min_q + 1;
        (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->q_count = q_count;

        g_tm_restore_ctx[dev]->q_profile_use_cnt++;
        (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->in_use = true;
      }
    }
  }

  return (rc);
}

static bf_status_t bf_tm_restore_q_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  int j, lport, q, q_profile_index;
  bf_dev_port_t port;
  uint32_t num_pipes;
  bf_tm_eg_q_t *_q;
  bf_tm_port_t *_p;
  bf_tm_q_profile_t *q_prof;

  LOG_TRACE("%s:", __func__);

  rc = bf_tm_restore_qac_offset_profile(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore qac offset profile table cfg");
    return (rc);
  }

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (lport = 0; lport < g_tm_restore_ctx[dev]->tm_cfg.ports_per_pg *
                                g_tm_restore_ctx[dev]->tm_cfg.pg_per_pipe;
         lport++) {
      port = MAKE_DEV_PORT(j, lport);
      _p = BF_TM_PORT_PTR(g_tm_restore_ctx[dev], port);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to get port descriptor pipe %d, port %d", j, lport);
        return (rc);
      }
      // Walk all queues behind a port and restore cfg.
      // How is q_count set in restored ctx ???
      for (q = 0;
           q < (g_tm_restore_ctx[dev]->q_profile + _p->qid_profile)->q_count;
           q++) {
        q_prof = g_tm_restore_ctx[dev]->q_profile;
        q_profile_index =
            (BF_TM_PORT_PTR(g_tm_restore_ctx[dev], port))->qid_profile;
        q_prof += q_profile_index;
        _q = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_restore_ctx[dev], port) +
             q_prof->base_q + q;

        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to queue descriptor for pipe %d, port %d, q %d",
                    j,
                    lport,
                    q);
          return (rc);
        }

        // For hitless warm init we need to add back the Queue cache counters
        // as they are deleted when the device is deleted
        rc = bf_tm_q_set_cache_counters(dev, port, q);
        if (rc != BF_SUCCESS) {
          LOG_ERROR(
              "TM: %s:%d bf_tm_q_set_cache_counters failed for dev %d, port "
              "%d, "
              "queue "
              "%d, rc "
              "%s (%d)",
              __func__,
              __LINE__,
              dev,
              port,
              _q->physical_q,
              bf_err_str(rc),
              rc);
          return (rc);
        }

        _q->in_use = true;  // <---- Is it okay to set ???
        _q->port = lport;

        rc = bf_tm_q_get_min_limit(
            dev, _q, &(_q->thresholds.min_limit), &(_q->thresholds.min_limit));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue min limit");
          return (rc);
        }

        rc = bf_tm_q_get_fast_recovery_mode(dev,
                                            _q,
                                            &(_q->q_cfg.fast_recover_mode),
                                            &(_q->q_cfg.fast_recover_mode));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue fast recovery mode");
          return (rc);
        }

        rc = bf_tm_q_get_is_dynamic(
            dev, _q, &(_q->q_cfg.is_dynamic), &(_q->q_cfg.is_dynamic));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue dynamic or static usage");
          return (rc);
        }
        if (_q->q_cfg.is_dynamic) {
          rc = bf_tm_q_get_baf(dev, _q, &(_q->q_cfg.baf), &(_q->q_cfg.baf));
          if (rc != BF_SUCCESS) {
            LOG_ERROR("Unable to restore queue baf");
            return (rc);
          }
        }
        // restore is_dynamic before app limit.
        rc = bf_tm_q_get_app_limit(
            dev, _q, &(_q->thresholds.app_limit), &(_q->thresholds.app_limit));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue shared limit");
          return (rc);
        }
        rc = bf_tm_q_get_app_hyst(
            dev, _q, &(_q->thresholds.app_hyst), &(_q->thresholds.app_hyst));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue shared hysteresis limit");
          return (rc);
        }
        rc = bf_tm_q_get_app_hyst_index(dev,
                                        _q,
                                        &(_q->thresholds.app_hyst_index),
                                        &(_q->thresholds.app_hyst_index));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue hysteresis index");
          return (rc);
        }
        rc = bf_tm_q_get_yel_hyst(
            dev, _q, &(_q->thresholds.yel_hyst), &(_q->thresholds.yel_hyst));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue yellow color hysteresis");
          return (rc);
        }
        rc = bf_tm_q_get_yel_hyst_index(dev,
                                        _q,
                                        &(_q->thresholds.yel_hyst_index),
                                        &(_q->thresholds.yel_hyst_index));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue yellow color hysteresis index");
          return (rc);
        }
        rc = bf_tm_q_get_red_hyst(
            dev, _q, &(_q->thresholds.red_hyst), &(_q->thresholds.red_hyst));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue red color hysteresis");
          return (rc);
        }
        rc = bf_tm_q_get_red_hyst_index(dev,
                                        _q,
                                        &(_q->thresholds.red_hyst_index),
                                        &(_q->thresholds.red_hyst_index));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue red color hysteresis index");
          return (rc);
        }
        rc = bf_tm_q_get_yel_limit_pcent(dev,
                                         _q,
                                         &(_q->thresholds.yel_limit_pcent),
                                         &(_q->thresholds.yel_limit_pcent));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue yellow color limit");
          return (rc);
        }
        rc = bf_tm_q_get_red_limit_pcent(dev,
                                         _q,
                                         &(_q->thresholds.red_limit_pcent),
                                         &(_q->thresholds.red_limit_pcent));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue red color limit");
          return (rc);
        }

        rc = bf_tm_q_get_app_poolid(
            dev, _q, &(_q->q_cfg.app_poolid), &(_q->q_cfg.app_poolid));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue app poolid");
          return (rc);
        }
        rc = bf_tm_q_get_color_drop_en(
            dev, _q, &(_q->q_cfg.color_drop_en), &(_q->q_cfg.color_drop_en));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue color drop state");
          return (rc);
        }
        rc = bf_tm_q_get_tail_drop_en(
            dev, _q, &(_q->q_cfg.tail_drop_en), &(_q->q_cfg.tail_drop_en));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue tail drop state");
          return (rc);
        }
        // fast recover mode -- restore ?
        rc = bf_tm_sch_get_q_sched_prio(dev,
                                        _q,
                                        false,
                                        &(_q->q_sch_cfg.max_rate_sch_prio),
                                        &(_q->q_sch_cfg.max_rate_sch_prio));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue min rate scheduling priority");
          return (rc);
        }
        rc = bf_tm_sch_get_q_sched_prio(dev,
                                        _q,
                                        true,
                                        &(_q->q_sch_cfg.min_rate_sch_prio),
                                        &(_q->q_sch_cfg.min_rate_sch_prio));

        if (rc != BF_SUCCESS) {
          LOG_ERROR(
              "Unable to restore queue remaining bandwith rate scheduling "
              "priority");
          return (rc);
        }
        rc = bf_tm_sch_get_q_dwrr_wt(
            dev, _q, &(_q->q_sch_cfg.dwrr_wt), &(_q->q_sch_cfg.dwrr_wt));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue scheduling dwrr weights");
          return (rc);
        }
        rc = bf_tm_sch_get_q_pfc_prio(
            dev, _q, &(_q->q_sch_cfg.pfc_prio), &(_q->q_sch_cfg.pfc_prio));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue pfc priority");
          return (rc);
        }
        rc = bf_tm_sch_get_q_adv_fc_mode(dev,
                                         _q,
                                         &(_q->q_sch_cfg.adv_fc_mode),
                                         &(_q->q_sch_cfg.adv_fc_mode));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore advanced flow control mode");
          return (rc);
        }
        rc = bf_tm_sch_get_q_max_rate(dev,
                                      _q,
                                      &(_q->q_sch_cfg.pps),
                                      &(_q->q_sch_cfg.max_burst_size),
                                      &(_q->q_sch_cfg.max_burst_size),
                                      &(_q->q_sch_cfg.max_rate),
                                      &(_q->q_sch_cfg.max_rate));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue max rate, burst size");
          return (rc);
        }

        rc = bf_tm_sch_q_max_rate_status_get(
            dev, _q, &(_q->q_sch_cfg.max_rate_enable));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue max rate enable status");
          return (rc);
        }

        rc = bf_tm_sch_get_q_gmin_rate(dev,
                                       _q,
                                       &(_q->q_sch_cfg.pps),
                                       &(_q->q_sch_cfg.min_burst_size),
                                       &(_q->q_sch_cfg.min_burst_size),
                                       &(_q->q_sch_cfg.min_rate),
                                       &(_q->q_sch_cfg.min_rate));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue min rate");
          return (rc);
        }

        rc = bf_tm_sch_q_min_rate_status_get(
            dev, _q, &(_q->q_sch_cfg.min_rate_enable));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue min rate enable status");
          return (rc);
        }

        rc = bf_tm_sch_get_q_sched(dev,
                                   _q,
                                   &(_q->q_sch_cfg.sch_enabled),
                                   &(_q->q_sch_cfg.sch_enabled));
        if (rc != BF_SUCCESS) {
          LOG_ERROR("Unable to restore queue scheduling state");
          return (rc);
        }
      }
    }
  }
  return (rc);
}

static bf_status_t bf_tm_restore_pool_cfg(bf_dev_id_t dev) {
  int j, c, pool;
  bf_tm_status_t rc = BF_TM_EOK;
  LOG_TRACE("%s:", __func__);
  // Ingress Pools
  for (pool = BF_TM_IG_APP_POOL_0; pool < g_tm_ctx[dev]->tm_cfg.shared_pool_cnt;
       pool++) {
    rc = bf_tm_ig_spool_get_red_limit(
        dev,
        pool,
        g_tm_restore_ctx[dev]->ig_pool,
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.red_limit),
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.red_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d red color threshold limit",
                pool);
      return (rc);
    }
    rc = bf_tm_ig_spool_get_yel_limit(
        dev,
        pool,
        g_tm_restore_ctx[dev]->ig_pool,
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.yel_limit),
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.yel_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d yel color threshold limit",
                pool);
      ;
      return (rc);
    }
    rc = bf_tm_ig_spool_get_green_limit(
        dev,
        pool,
        g_tm_restore_ctx[dev]->ig_pool,
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.green_limit),
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.green_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d green color threshold limit",
                pool);
      ;
      return (rc);
    }

    for (c = 0; c < BF_TM_MAX_COS_LEVELS; c++) {
      rc = bf_tm_ig_spool_get_pfc_limit(
          dev,
          pool,
          c,
          g_tm_restore_ctx[dev]->ig_pool,
          &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].threshold.pfc_limit[c]),
          &(g_tm_restore_ctx[dev]
                ->ig_pool->spool[pool]
                .threshold.pfc_limit[c]));
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "Unable to restore shared pool %d pfc threshold limit for icos %d",
            pool,
            c);
        return (rc);
      }
    }
    rc = bf_tm_ig_spool_get_color_drop(
        dev,
        pool,
        g_tm_restore_ctx[dev]->ig_pool,
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].color_drop_en),
        &(g_tm_restore_ctx[dev]->ig_pool->spool[pool].color_drop_en));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d color drop config state",
                pool);
      return (rc);
    }
  }
  rc = bf_tm_ig_spool_get_green_hyst(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->green_hyst),
      &(g_tm_restore_ctx[dev]->ig_pool->green_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore shared pool green hysteresis limit");
    return (rc);
  }
  rc = bf_tm_ig_spool_get_yel_hyst(dev,
                                   g_tm_restore_ctx[dev]->ig_pool,
                                   &(g_tm_restore_ctx[dev]->ig_pool->yel_hyst),
                                   &(g_tm_restore_ctx[dev]->ig_pool->yel_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore shared pool yellow hysteresis limit");
    ;
    return (rc);
  }
  rc = bf_tm_ig_spool_get_red_hyst(dev,
                                   g_tm_restore_ctx[dev]->ig_pool,
                                   &(g_tm_restore_ctx[dev]->ig_pool->red_hyst),
                                   &(g_tm_restore_ctx[dev]->ig_pool->red_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore shared pool red hysteresis limit");
    return (rc);
  }

  rc = bf_tm_ig_gpool_get_dod_limit(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.dod_limit),
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.dod_limit));
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "Unable to restore ingress negative mirror / deflection drop limit");
    return (rc);
  }
  rc = bf_tm_ig_gpool_get_skid_limit(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.skid_limit),
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.skid_limit));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore skid pool threshold limit");
    return (rc);
  }
  rc = bf_tm_ig_gpool_get_skid_hyst(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.skid_hyst),
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.skid_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore skid pool hysteresis limit");
    return (rc);
  }
  rc = bf_tm_ig_gpool_get_glb_min_limit(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.glb_min_limit),
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.glb_min_limit));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore global min limit");
    return (rc);
  }
  rc = bf_tm_ig_gpool_get_glb_cell_limit(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.glb_cell_limit),
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.glb_cell_limit));
  if (rc != BF_NOT_SUPPORTED && rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore global cell limit");
    return (rc);
  }
  rc = bf_tm_ig_gpool_get_glb_cell_limit_state(
      dev,
      g_tm_restore_ctx[dev]->ig_pool,
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.glb_cell_limit_enable),
      &(g_tm_restore_ctx[dev]->ig_pool->gpool.glb_cell_limit_enable));
  if (rc != BF_NOT_SUPPORTED && rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore global cell limit state");
    return (rc);
  }

  // Egress Pools
  for (j = BF_TM_EG_APP_POOL_0;
       j < BF_TM_EG_APP_POOL_0 + g_tm_ctx[dev]->tm_cfg.shared_pool_cnt;
       j++) {
    pool = j - BF_TM_EG_APP_POOL_0;
    rc = bf_tm_eg_spool_get_red_limit(
        dev,
        pool,
        g_tm_restore_ctx[dev]->eg_pool,
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)->threshold.red_limit),
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)->threshold.red_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d red color threshold limit",
                pool);
      return (rc);
    }
    rc = bf_tm_eg_spool_get_yel_limit(
        dev,
        pool,
        g_tm_restore_ctx[dev]->eg_pool,
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)->threshold.yel_limit),
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)->threshold.yel_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d red color threshold limit",
                pool);
      return (rc);
    }
    rc = bf_tm_eg_spool_get_green_limit(
        dev,
        pool,
        g_tm_restore_ctx[dev]->eg_pool,
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)
              ->threshold.green_limit),
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)
              ->threshold.green_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore shared pool %d red color threshold limit",
                pool);
      return (rc);
    }
    rc = bf_tm_eg_spool_get_color_drop(
        dev,
        pool,
        g_tm_restore_ctx[dev]->eg_pool,
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)->color_drop_en),
        &((g_tm_restore_ctx[dev]->eg_pool->spool + pool)->color_drop_en));
    if (rc != BF_SUCCESS) {
      return (rc);
    }
  }
  rc = bf_tm_eg_spool_get_green_hyst(
      dev,
      g_tm_restore_ctx[dev]->eg_pool,
      &(g_tm_restore_ctx[dev]->eg_pool->green_hyst),
      &(g_tm_restore_ctx[dev]->eg_pool->green_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore shared pool green hysteresis limit");
    return (rc);
  }
  rc = bf_tm_eg_spool_get_yel_hyst(dev,
                                   g_tm_restore_ctx[dev]->eg_pool,
                                   &(g_tm_restore_ctx[dev]->eg_pool->yel_hyst),
                                   &(g_tm_restore_ctx[dev]->eg_pool->yel_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore shared pool yellow hysteresis limit");
    return (rc);
  }
  rc = bf_tm_eg_spool_get_red_hyst(dev,
                                   g_tm_restore_ctx[dev]->eg_pool,
                                   &(g_tm_restore_ctx[dev]->eg_pool->red_hyst),
                                   &(g_tm_restore_ctx[dev]->eg_pool->red_hyst));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore shared pool red hysteresis limit");
    return (rc);
  }

  rc = bf_tm_eg_gpool_get_dod_limit(
      dev,
      g_tm_restore_ctx[dev]->eg_pool,
      &(g_tm_restore_ctx[dev]->eg_pool->gpool.dod_limit),
      &(g_tm_restore_ctx[dev]->eg_pool->gpool.dod_limit));
  if (rc != BF_SUCCESS) {
    LOG_ERROR(
        "Unable to restore egress negative mirror / deflection drop limit");
    return (rc);
  }

  for (uint8_t pipe = 0; pipe < g_tm_restore_ctx[dev]->tm_cfg.pipe_cnt;
       pipe++) {
    for (uint8_t fifo = 0;
         fifo < g_tm_restore_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
         fifo++) {
      rc = bf_tm_eg_gpool_get_fifo_limit(
          dev,
          g_tm_restore_ctx[dev]->eg_pool,
          (bf_dev_pipe_t)pipe,
          fifo,
          &(g_tm_restore_ctx[dev]->eg_pool->gpool.fifo_limit[pipe][fifo]),
          &(g_tm_restore_ctx[dev]->eg_pool->gpool.fifo_limit[pipe][fifo]));
      if (rc != BF_SUCCESS) {
        LOG_ERROR(
            "TM: %s:%d Unable to restore egress fifo limit for dev %d, pipe "
            "%d, "
            "fifo "
            "%d, rc "
            "%s (%d)",
            __func__,
            __LINE__,
            dev,
            pipe,
            fifo,
            bf_err_str(rc),
            rc);
        return (rc);
      }
    }
  }

  rc = bf_tm_uc_ct_size_get(dev,
                            &g_tm_restore_ctx[dev]->uc_ct_size,
                            &g_tm_restore_ctx[dev]->uc_ct_size);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore unicast cut through size");
    return (rc);
  }
  rc = bf_tm_mc_ct_size_get(dev,
                            &g_tm_restore_ctx[dev]->mc_ct_size,
                            &g_tm_restore_ctx[dev]->mc_ct_size);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore multicast cut through size");
    return (rc);
  }

  return (rc);
}

static bf_status_t bf_tm_restore_pipe_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  int j;
  uint32_t num_pipes;
  bf_tm_eg_pipe_t *p;

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    p = g_tm_restore_ctx[dev]->pipes + j;
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to get pipe descriptor for pipe %d", j);
      return (rc);
    }
    rc = bf_tm_pipe_get_limit(
        dev,
        p,
        &((g_tm_restore_ctx[dev]->pipes + j)->epipe_limit),
        &((g_tm_restore_ctx[dev]->pipes + j)->epipe_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore pipe threshold limit for pipe %d", j);
      return (rc);
    }
    rc = bf_tm_pipe_get_hyst(
        dev,
        p,
        &((g_tm_restore_ctx[dev]->pipes + j)->epipe_resume_limit),
        &((g_tm_restore_ctx[dev]->pipes + j)->epipe_resume_limit));
    if (rc != BF_SUCCESS) {
      LOG_ERROR("Unable to restore pipe hysteresis for pipe %d", j);
      return (rc);
    }
    rc = bf_tm_sch_get_pkt_ifg_compensation(
        dev,
        j,
        &((g_tm_restore_ctx[dev]->pipes + j)->ifg_compensation),
        &((g_tm_restore_ctx[dev]->pipes + j)->ifg_compensation));
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "Unable to restore inter frame gap compensation config for pipe %d",
          j);
      return (rc);
    }
    rc = bf_tm_q_get_mirror_on_drop_destination(
        dev,
        j,
        &((g_tm_restore_ctx[dev]->pipes + j)->neg_mirror_dest),
        &((g_tm_restore_ctx[dev]->pipes + j)->neg_mirror_dest));
    if (rc != BF_SUCCESS) {
      LOG_ERROR(
          "Unable to restore negative mirror destination config for pipe %d",
          j);
      return (rc);
    }
  }

  rc = bf_tm_timestamp_shift_get(dev,
                                 &((g_tm_restore_ctx[dev])->timestamp_shift));
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore timestamp shift");
    return (rc);
  }
  return (rc);
}

static bf_status_t bf_tm_restore_mcast_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  LOG_TRACE("%s:", __func__);

  bf_tm_dev_cfg_t tm_cfg;
  rc = bf_tm_dev_config_get(dev, &tm_cfg);
  if (BF_SUCCESS != rc) {
    LOG_ERROR("Unable to get TM cfg for MC restoration");
    return rc;
  }

  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (int pipe = 0; pipe < (int)num_pipes; pipe++) {
    for (int fifo = 0; fifo < tm_cfg.pre_fifo_per_pipe; fifo++) {
      bf_tm_mcast_t *fifo_ptr = g_tm_restore_ctx[dev]->mcast_fifo +
                                (pipe * tm_cfg.pre_fifo_per_pipe) + fifo;
      rc = bf_tm_mc_fifo_icos_mapping_get(
          dev, pipe, fifo, &(fifo_ptr->icos_bmap));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore pipe %d fifo %d icos bitmap", pipe, fifo);
        return (rc);
      }

      rc = bf_tm_mc_fifo_arb_mode_get(dev, pipe, fifo, &(fifo_ptr->arb_mode));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore pipe %d fifo %d arb_mode", pipe, fifo);
        return (rc);
      }

      rc = bf_tm_mc_fifo_wrr_weight_get(dev, pipe, fifo, &(fifo_ptr->weight));
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore pipe %d fifo %d wrr_weight", pipe, fifo);
        return (rc);
      }

      int size = 0;
      rc = bf_tm_mc_fifo_depth_get(dev, pipe, fifo, &size);
      if (rc != BF_SUCCESS) {
        LOG_ERROR("Unable to restore pipe %d fifo %d depth", pipe, fifo);
        return (rc);
      }
      fifo_ptr->size = (uint16_t)size;
    }
  }
  return (rc);
}

bf_status_t bf_tm_restore_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;

  TM_LOCK(dev, g_tm_restore_ctx[dev]->lock);
  // Set internal_call so that TM_LOCK is moot
  g_tm_restore_ctx[dev]->internal_call = true;

  // Set up HW update functional table to Read-Only and NULL write function
  // pointers. This will ensure that values read from HW are populated into
  // sw context structures (per submodule) but not repushed to HW.
  bf_tm_ppg_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_q_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_ig_pool_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_eg_pool_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_port_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_sch_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_pipe_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_mcast_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_path_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);
  bf_tm_dev_set_hw_ftbl_cfg_restore(g_tm_restore_ctx[dev]);

  // Invalidate WAC/QAC cache.
  g_tm_restore_ctx[dev]->read_por_wac_profile_table = false;
  g_tm_restore_ctx[dev]->read_por_qac_profile_table = false;

  rc = bf_tm_restore_pool_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore Pool cfg");
    goto cleanup;
  }

  rc = bf_tm_restore_ppg_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore PPG cfg");
    goto cleanup;
  }

  rc = bf_tm_restore_q_profile_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore Q profile config");
    goto cleanup;
  }
  rc = bf_tm_restore_port_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore Port cfg");
    goto cleanup;
  }
  // !! Restore Q config after restoring q-profile config. !!
  // Before restoring Q cfg, port to q mapping should be in place.
  rc = bf_tm_restore_q_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore Q cfg");
    goto cleanup;
  }

  // Restore pipe cfg after port and q cfg are restored
  rc = bf_tm_restore_pipe_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore Pipe cfg");
    goto cleanup;
  }

  rc = bf_tm_restore_mcast_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("TM: Unable to restore TM mcast cfg");
    goto cleanup;
  }

  // At the end of config restore from HW, update ftbl
  // to do both read and write operations to ASIC.
  bf_tm_ppg_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_q_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_ig_pool_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_eg_pool_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_port_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_sch_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_pipe_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_mcast_set_hw_ftbl(g_tm_restore_ctx[dev]);
  bf_tm_path_set_hw_ftbl(g_tm_restore_ctx[dev]);

cleanup:
  g_tm_restore_ctx[dev]->internal_call = false;
  TM_UNLOCK(dev, g_tm_restore_ctx[dev]->lock);

  return (rc);
}

bf_tm_status_t bf_tm_restore_dev_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  rc = bf_tm_restore_cfg(dev);
  if (rc != BF_SUCCESS) {
    LOG_ERROR("Unable to restore configuration by reading back device");
    return (rc);
  }
  return (rc);
}

bf_tm_status_t bf_tm_restore_device_cfg(bf_dev_id_t dev) {
  // Use asic_type to distinguish tofino/tofino-lite/future asic..
  // for now using TOFINO without checking asic_type
  bf_tm_status_t rc = BF_TM_EOK;
  // for regular hitless HA, make restore_ctx and device_ctx same.
  g_tm_restore_ctx[dev] = g_tm_ctx[dev];

  rc = bf_tm_restore_dev_cfg(dev);
  if (rc == BF_SUCCESS) {
    LOG_TRACE("Hitless HA: TM Cfg successfully restored.");
  } else {
    // free up TM ctx memory.
    LOG_TRACE("Hitless HA: TM Cfg restoration failed.");
    g_tm_restore_ctx[dev] = NULL;
  }
  return (rc);
}
