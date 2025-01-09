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
#include <stdlib.h>
#include <inttypes.h>
#include <dvm/bf_drv_intf.h>
#include "tm_ctx.h"
#include "tm_init.h"
#include "tm_hw_access.h"
#include "traffic_mgr/init/tm_tofino.h"
#include <lld/lld_err.h>
#include <lld/lld_sku.h>
#include <traffic_mgr/tm_intf.h>
#include <target-sys/bf_sal/bf_sys_intf.h>
#include <lld/bf_dev_if.h>

/*
 *             This file has code meant for Unit Testing
 *
 *    This file has code to verify restored TM configuration with original
 *    TM config. Inorder to verify, original TM config is kept intact (driver
 *    context is not wiped or lost; driver is not restarted). A unit test
 *    signal (ucli/api) triggers driver to rebuild TM config in new memory
 *    area. This newly rebuit TM config is compared with original TM config.
 */

// Create another instance of TM device and restore config from HW
// into this new device. Used only for UT purposes.
static bf_tm_status_t bf_tm_init_restore_device(bf_dev_id_t dev,
                                                bf_tm_asic_en asic_type) {
  bf_tm_status_t rc = BF_TM_EOK;

  rc = bf_tm_create_tm_ctx(
      asic_type, dev, BF_TM_CTX_RESTORED, &g_tm_restore_ctx[dev]);
  if (BF_TM_IS_NOTOK(rc)) {
    LOG_ERROR("TM: Unable to create TM context");
    return (rc);
  }
  rc = bf_tm_construct_modules(g_tm_restore_ctx[dev]);
  if (BF_TM_IS_NOTOK(rc)) {
    // Initiate destruction
    LOG_ERROR("TM: Unable to create sub-modules of TM");
    bf_tm_destruct_tm_ctx(g_tm_restore_ctx[dev]);
    return (rc);
  }
  return (rc);
}

#define BF_TM_CFG_VERIFY_FAIL(str, cfgname, restore_cfg, orig_cfg)       \
  {                                                                      \
    LOG_ERROR(                                                           \
        "FAIL: %s, Cfg name %s : restored cfg value = %d, original cfg " \
        "value = %d",                                                    \
        str,                                                             \
        cfgname,                                                         \
        restore_cfg,                                                     \
        orig_cfg);                                                       \
    rc |= BF_TM_EINT;                                                    \
  }

#define BF_TM_CFG_VERIFY_PASS(str, cfgname, restore_cfg, orig_cfg)          \
  {                                                                         \
    LOG_TRACE(                                                              \
        "PASS: %s, Cfg name %s : Restored cfg and original cfg value = %d", \
        str,                                                                \
        cfgname,                                                            \
        restore_cfg);                                                       \
  }

#define BF_TM_CFG_VERIFY_SKIP(str, cfgname)                                    \
  {                                                                            \
    LOG_TRACE(                                                                 \
        "SKIP: %s, Cfg name %s : skipped because this check is not available " \
        "on model",                                                            \
        str,                                                                   \
        cfgname);                                                              \
  }

/*
**** TOF1 available fields to check on Model
*/
const char *const t1_ppg_avail[] = {"ppipe",
                                    "lpipe",
                                    "ppg#",
                                    "port",
                                    "poolid",
                                    "icosmask",
                                    "is_pfc",
                                    "hysteresis",
                                    NULL};
const char *const t1_q_prof_avail[] = {
    "use_count", "q_inuse", "baseq", "qcount", "ch_in_pg", "qmapping", NULL};
const char *const t1_port_avail[] = {"pipe",
                                     "lpipe",
                                     "port",
                                     "pg",
                                     "mirrorport",
                                     "wacdroplimit",
                                     "qacdroplimit",
                                     "wacresumelimit",
                                     "qacresumelimit",
                                     "ct_enabled",
                                     "qid_profile#",
                                     "pps",
                                     "max_rate_enabled",
                                     "tdm",
                                     "fctype",
                                     "icos-to-cos",
                                     "ppgs",
                                     "cos_enable",
                                     NULL};
const char *const t1_q_avail[] = {"ppipe",
                                  "lpipe",
                                  "pg",
                                  "port",
                                  "logical_q",
                                  "physical_q",
                                  "app_poolid",
                                  "colordrop",
                                  "fastrecovermode",
                                  "red_hyst",
                                  "yel_hyst",
                                  "sch_enabled",
                                  "sch_pfc_enabled",
                                  "pps",
                                  "cid",
                                  "pfc_sch_prio",
                                  "max_rate_sch_prio",
                                  "min_rate_sch_prio",
                                  NULL};
const char *const t1_pipe_avail[] = {"p_pipe",
                                     "l_pipe",
                                     "resume_limit",
                                     "ifg_comp",
                                     "neg_mirror_ppipe",
                                     "neg_mirror_port",
                                     "neg_mirror_phy_q",
                                     NULL};
const char *const t1_mc_avail[] = {
    "pipe", "fifo", "icos bmap", "arb_mode", "wrr_weight", "depth", NULL};

/*
**** TOF2 available fields to check on Model
*/

/*
**** TOF3 available fields to check on Model
*/

static bool is_in(char *keyword, const char *const *list) {
  while (NULL != *list) {
    if (0 == strcmp(*list, keyword)) {
      return true;
    }
    ++list;
  }
  return false;
}

static bool bf_tm_cfg_check_available_on_model(bf_dev_id_t dev,
                                               const char *caller,
                                               char *cfgname) {
  if (BF_TM_IS_TOFINO(g_tm_ctx[dev]->asic_type)) {
    if (0 == strcmp(caller, "bf_tm_verify_ppg_cfg")) {
      return is_in(cfgname, t1_ppg_avail);
    } else if (0 == strcmp(caller, "bf_tm_verify_q_profile")) {
      return is_in(cfgname, t1_q_prof_avail);
    } else if (0 == strcmp(caller, "bf_tm_verify_port_cfg")) {
      return is_in(cfgname, t1_port_avail);
    } else if (0 == strcmp(caller, "bf_tm_verify_q_cfg")) {
      return is_in(cfgname, t1_q_avail);
    } else if (0 == strcmp(caller, "bf_tm_verify_pipe_cfg")) {
      return is_in(cfgname, t1_pipe_avail);
    } else if (0 == strcmp(caller, "bf_tm_verify_mc_cfg")) {
      return is_in(cfgname, t1_mc_avail);
    }

  } else if (BF_TM_IS_TOF2(g_tm_ctx[dev]->asic_type)) {
    return true;
  } else if (BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    return true;
  } else {
    return false;
  }
  return false;
}

#define BF_TM_CFG_CHECK_FAIL(str, cfgname, restore_cfg, orig_cfg)       \
  {                                                                     \
    if (TM_IS_TARGET_ASIC(dev)) {                                       \
      BF_TM_CFG_VERIFY_FAIL(str, cfgname, restore_cfg, orig_cfg)        \
    } else {                                                            \
      if (bf_tm_cfg_check_available_on_model(dev, __func__, cfgname)) { \
        BF_TM_CFG_VERIFY_FAIL(str, cfgname, restore_cfg, orig_cfg)      \
      } else {                                                          \
        BF_TM_CFG_VERIFY_SKIP(str, cfgname)                             \
      }                                                                 \
    }                                                                   \
  }

#define BF_TM_CFG_VERIFY(str, cfgname, restore_cfg, orig_cfg)    \
  {                                                              \
    if (restore_cfg != orig_cfg) {                               \
      BF_TM_CFG_CHECK_FAIL(str, cfgname, restore_cfg, orig_cfg)  \
    } else {                                                     \
      BF_TM_CFG_VERIFY_PASS(str, cfgname, restore_cfg, orig_cfg) \
    }                                                            \
  }

#define BF_TM_CFG_LVERIFY(str, cfgname, restore_cfg, orig_cfg)             \
  {                                                                        \
    if (restore_cfg != orig_cfg) {                                         \
      LOG_ERROR("FAIL: %s, Cfg name %s : restored cfg value = %" PRIu64    \
                ", original cfg "                                          \
                "value = %" PRIu64 "",                                     \
                str,                                                       \
                cfgname,                                                   \
                restore_cfg,                                               \
                orig_cfg);                                                 \
      rc |= BF_TM_EINT;                                                    \
    } else {                                                               \
      LOG_TRACE(                                                           \
          "PASS: %s, Cfg name %s : Restored cfg and original cfg value = " \
          "%" PRIu64 "",                                                   \
          str,                                                             \
          cfgname,                                                         \
          restore_cfg);                                                    \
    }                                                                      \
  }

#define BF_TM_CFG_VERIFY2(str, cfgname, restore_cfg, orig_cfg, error_margin) \
  {                                                                          \
    if ((uint32_t)(abs((int)orig_cfg - (int)restore_cfg)) <=                 \
        ((orig_cfg * error_margin) / 100)) {                                 \
      BF_TM_CFG_VERIFY_PASS(str, cfgname, restore_cfg, orig_cfg)             \
    } else {                                                                 \
      BF_TM_CFG_CHECK_FAIL(str, cfgname, restore_cfg, orig_cfg)              \
    }                                                                        \
  }

#define BF_TM_CFG_VERIFY3(str, cfgname, restore_cfg, orig_cfg)    \
  {                                                               \
    if ((orig_cfg == restore_cfg) ||                              \
        (((uint32_t)(abs((int)orig_cfg - (int)restore_cfg)) <=    \
          BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS) &&                 \
         (restore_cfg == BF_TM_TOFINO_WAC_DEFAULT_HYSTERESIS))) { \
      BF_TM_CFG_VERIFY_PASS(str, cfgname, restore_cfg, orig_cfg)  \
    } else {                                                      \
      BF_TM_CFG_CHECK_FAIL(str, cfgname, restore_cfg, orig_cfg)   \
    }                                                             \
  }

#define BF_TM_CFG_VERIFY4(str, cfgname, restore_cfg, orig_cfg)    \
  {                                                               \
    if ((orig_cfg == restore_cfg) ||                              \
        (((uint32_t)(abs((int)orig_cfg - (int)restore_cfg)) <=    \
          BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS) &&                 \
         (restore_cfg == BF_TM_TOFINO_QAC_DEFAULT_HYSTERESIS))) { \
      BF_TM_CFG_VERIFY_PASS(str, cfgname, restore_cfg, orig_cfg)  \
    } else {                                                      \
      BF_TM_CFG_CHECK_FAIL(str, cfgname, restore_cfg, orig_cfg)   \
    }                                                             \
  }

#define TEST_CASE_COUNT 6

static bf_tm_status_t bf_tm_verify_ppg_cfg(bf_dev_id_t dev) {
  int j, k;
  bf_tm_ppg_t *_ppg, *ppg;
  bf_tm_status_t rc = BF_TM_EOK;
  char str[200];
  uint32_t num_pipes;

  LOG_TRACE("%s:", __func__);

  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (k = 0; k < g_tm_ctx[dev]->tm_cfg.total_ppg_per_pipe; k++) {
      _ppg = BF_TM_PPG_PTR(g_tm_restore_ctx[dev], j, k);
      BF_TM_PPG_CHECK(_ppg, k, j, dev);
      ppg = BF_TM_PPG_PTR(g_tm_ctx[dev], j, k);
      BF_TM_PPG_CHECK(ppg, k, j, dev);

      sprintf(str, "PPG Cfg: Pipe = %d, ppg = %d", j, k);
      BF_TM_CFG_VERIFY(str, "ppipe", _ppg->p_pipe, ppg->p_pipe);
      BF_TM_CFG_VERIFY(str, "lpipe", _ppg->l_pipe, ppg->l_pipe);
      BF_TM_CFG_VERIFY(str, "ppg#", _ppg->ppg, ppg->ppg);
      // This can be set only by replaying TM PPG allocation API
      // BF_TM_CFG_VERIFY(str, "ppginuse", _ppg->in_use, ppg->in_use);
      BF_TM_CFG_VERIFY(str, "port", _ppg->port, ppg->port);
      BF_TM_CFG_VERIFY(
          str, "poolid", _ppg->ppg_cfg.app_poolid, ppg->ppg_cfg.app_poolid);
      BF_TM_CFG_VERIFY(
          str, "icosmask", _ppg->ppg_cfg.icos_mask, ppg->ppg_cfg.icos_mask);

      if (!(ppg->is_default_ppg)) {
        BF_TM_CFG_VERIFY(str,
                         "isdynamic",
                         _ppg->ppg_cfg.is_dynamic,
                         ppg->ppg_cfg.is_dynamic);
        BF_TM_CFG_VERIFY(str, "baf", _ppg->ppg_cfg.baf, ppg->ppg_cfg.baf);
        BF_TM_CFG_VERIFY(str,
                         "minlimit",
                         _ppg->thresholds.min_limit,
                         ppg->thresholds.min_limit);
        BF_TM_CFG_VERIFY(str,
                         "applimit",
                         _ppg->thresholds.app_limit,
                         ppg->thresholds.app_limit);
        BF_TM_CFG_VERIFY(str,
                         "hyst-index",
                         _ppg->thresholds.hyst_index,
                         ppg->thresholds.hyst_index);
        BF_TM_CFG_VERIFY(str,
                         "skidlimit",
                         _ppg->thresholds.skid_limit,
                         ppg->thresholds.skid_limit);
      }

      BF_TM_CFG_VERIFY(
          str, "is_pfc", _ppg->ppg_cfg.is_pfc, ppg->ppg_cfg.is_pfc);

      // because of hw store hysteresis value in 8-cell units and sw in cells,
      // lossy conversion missmatch can occur
      BF_TM_CFG_VERIFY3(str,
                        "hysteresis",
                        _ppg->thresholds.ppg_hyst,
                        ((ppg->thresholds.ppg_hyst >> 3) << 3));
    }
  }
  return (rc);
}

static bf_tm_status_t bf_tm_verify_q_profile(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;

  if (BF_TM_IS_TOFINO(g_tm_ctx[dev]->asic_type)) {
    int i, k;
    char str[200];
    bf_tm_q_profile_t *q_prof, *q_restored_prof;

    LOG_TRACE("%s:", __func__);

    q_prof = g_tm_ctx[dev]->q_profile;
    q_restored_prof = g_tm_restore_ctx[dev]->q_profile;

    // Verify the queue profile use count
    BF_TM_CFG_VERIFY("Q-Profile",
                     "use_count",
                     g_tm_restore_ctx[dev]->q_profile_use_cnt,
                     g_tm_ctx[dev]->q_profile_use_cnt);

    for (i = 0; i < g_tm_ctx[dev]->tm_cfg.q_prof_cnt; i++) {
      sprintf(str, "Q-Profile = %d", i);
      BF_TM_CFG_VERIFY(
          str, "q_inuse", (q_restored_prof + i)->in_use, (q_prof + i)->in_use);
      // Skip rest of the verification if the q profile is not in use
      if (!(q_restored_prof + i)->in_use) {
        continue;
      }

      BF_TM_CFG_VERIFY(
          str, "baseq", (q_restored_prof + i)->base_q, (q_prof + i)->base_q);
      BF_TM_CFG_VERIFY(
          str, "qcount", (q_restored_prof + i)->q_count, (q_prof + i)->q_count);
      BF_TM_CFG_VERIFY(str,
                       "ch_in_pg",
                       (q_restored_prof + i)->ch_in_pg,
                       (q_prof + i)->ch_in_pg);
      for (k = 0; k < g_tm_ctx[dev]->tm_cfg.q_per_pg; k++) {
        sprintf(str, "Q-Profile = %d, Q in PG %d", i, k);
        BF_TM_CFG_LVERIFY(str,
                          "qmapping",
                          (q_restored_prof + i)->q_mapping[k],
                          (q_prof + i)->q_mapping[k]);
      }
    }
  } else if (BF_TM_IS_TOF2(g_tm_ctx[dev]->asic_type)) {
    // TOF2 has no q-profiles itself, add different code here
  } else if (BF_TM_IS_TOF3(g_tm_ctx[dev]->asic_type)) {
    // TOF3 has no q-profiles itself, add different code here
  }
  return (rc);
}

static bf_tm_status_t bf_tm_verify_port_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  char str[200];
  uint32_t num_pipes;
  int i, j, k;
  bf_dev_port_t port;
  bf_tm_port_t *_pd, *pd;
  bool result;

  LOG_TRACE("%s:", __func__);
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (k = 0; k < g_tm_ctx[dev]->tm_cfg.ports_per_pg *
                        g_tm_ctx[dev]->tm_cfg.pg_per_pipe;
         k++) {
      sprintf(str, "Port-Cfg: Pipe = %d, Port = %d", j, k);
      port = MAKE_DEV_PORT(j, k);
      _pd = BF_TM_PORT_PTR(g_tm_restore_ctx[dev], port);
      pd = BF_TM_PORT_PTR(g_tm_ctx[dev], port);
      BF_TM_CFG_VERIFY(str, "pipe", _pd->p_pipe, pd->p_pipe);
      BF_TM_CFG_VERIFY(str, "lpipe", _pd->l_pipe, pd->l_pipe);
      BF_TM_CFG_VERIFY(str, "port", _pd->port, pd->port);
      BF_TM_CFG_VERIFY(str, "pg", _pd->pg, pd->pg);
      BF_TM_CFG_VERIFY(str, "mirrorport", _pd->mirror_port, pd->mirror_port);
      // Cannot restore and verify port offline status.
      // BF_TM_CFG_VERIFY(str, "offline", _pd->offline, pd->offline);
      // Cannot restore and verify port ppgcount.
      // BF_TM_CFG_VERIFY(str, "ppgcount", _pd->ppg_count, pd->ppg_count);
      // wac drop limit stored as 8-cell units
      BF_TM_CFG_VERIFY(str,
                       "wacdroplimit",
                       _pd->wac_drop_limit,
                       TM_CLEAR_LOW_3_BITS(pd->wac_drop_limit));
      BF_TM_CFG_VERIFY(
          str, "qacdroplimit", _pd->qac_drop_limit, pd->qac_drop_limit);
      // wac hyst stored as 8-cell units
      BF_TM_CFG_VERIFY3(str,
                        "wacresumelimit",
                        _pd->wac_resume_limit,
                        TM_CLEAR_LOW_3_BITS(pd->wac_resume_limit));
      // qac hyst stored as 8-cell units
      BF_TM_CFG_VERIFY3(str,
                        "qacresumelimit",
                        _pd->qac_resume_limit,
                        TM_CLEAR_LOW_3_BITS(pd->qac_resume_limit));
      BF_TM_CFG_VERIFY(
          str, "WAC hystindex", _pd->wac_hyst_index, pd->wac_hyst_index);
      BF_TM_CFG_VERIFY(
          str, "QAC hystindex", _pd->qac_hyst_index, pd->qac_hyst_index);
      BF_TM_CFG_VERIFY(str, "ct_enabled", _pd->ct_enabled, pd->ct_enabled);
      BF_TM_CFG_VERIFY(str, "qid_profile#", _pd->qid_profile, pd->qid_profile);
      BF_TM_CFG_VERIFY(str, "pps", _pd->pps, pd->pps);

      /*
       * Convert burst sizes to mantissa-exponent form and verify
       * as both values may be different but after conversion it
       * should be same.
       */
      result =
          bf_tm_sch_cfg_verify_burst_size(dev, _pd->burst_size, pd->burst_size);
      if (result == true) {
        BF_TM_CFG_VERIFY_PASS(
            str, "burstsize", _pd->burst_size, pd->burst_size);
      } else {
        BF_TM_CFG_VERIFY_FAIL(
            str, "burstsize", _pd->burst_size, pd->burst_size);
      }

      /*
       * Convert rates to mantissa-exponent form and verify
       * as both values may be different but after conversion it
       * should be same.
       */
      result = bf_tm_sch_cfg_verify_rate(
          dev, _pd->port_rate, pd->port_rate, pd->pps);
      if (result == true) {
        BF_TM_CFG_VERIFY_PASS(str, "portrate", _pd->port_rate, pd->port_rate);
      } else {
        BF_TM_CFG_VERIFY_FAIL(str, "portrate", _pd->port_rate, pd->port_rate);
      }
      BF_TM_CFG_VERIFY(str,
                       "uc_ct_limit",
                       _pd->uc_cut_through_limit,
                       pd->uc_cut_through_limit);
      BF_TM_CFG_VERIFY(str, "sch_enabled", _pd->sch_enabled, pd->sch_enabled);

      BF_TM_CFG_VERIFY(
          str, "max_rate_enabled", _pd->max_rate_enabled, pd->max_rate_enabled);
      BF_TM_CFG_VERIFY(str, "tdm", _pd->tdm, pd->tdm);
      // Cannot restore and verify port credit and speed.
      // BF_TM_CFG_VERIFY(str, "credit", _pd->credit, pd->credit);
      // BF_TM_CFG_VERIFY(str, "speed", _pd->speed, pd->speed);

      // check only on online ports because HW-set occurs only for online ports
      if (!(pd->offline)) {
        BF_TM_CFG_VERIFY(str, "fctype", _pd->fc_type, pd->fc_type);
        BF_TM_CFG_VERIFY(
            str, "icos-to-cos", _pd->icos_to_cos_mask, pd->icos_to_cos_mask);
      }
      for (i = 0; i < BF_TM_MAX_PFC_LEVELS; i++) {
        // ppg_list can be atmost 8 per port. Hence its okay to
        // loop BF_TM_MAX_PFC_LEVELS although subscripting using PFC level
        // is not readable.
        // ppgs are maintained on per PFC level basis
        // ppg_list gets updated during ppg allocate API for HA and hence
        // it is not verified here as part of UT
        if (_pd->ppgs[i] && pd->ppgs[i]) {
          BF_TM_CFG_VERIFY(str, "ppgs", _pd->ppgs[i]->ppg, pd->ppgs[i]->ppg);
        }
      }
      if (!(pd->offline)) {
        uint8_t hw_en_mask = 0;
        for (i = 0; i < BF_TM_MAX_COS_LEVELS; ++i) {
          bool en = false;
          result = bf_tm_port_pfc_enable_get(dev, port, i, &en);
          if (BF_SUCCESS == result) {
            hw_en_mask |= en ? ((uint8_t)1 << (_pd->cos_to_icos[i])) : 0;
          } else {
            LOG_ERROR("Unable to get PFC enable for pipe %d port %d cos_lvl %d",
                      pd->l_pipe,
                      pd->port,
                      i);
          }
        }
        // this is to verify the resulting mask itself, not each individual iCoS
        // to CoS item.
        BF_TM_CFG_VERIFY(str, "cos_enable", hw_en_mask, pd->icos_to_cos_mask);
      }
    }
  }
  return (rc);
}

static bf_tm_status_t bf_tm_verify_q_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  char str[200];
  uint32_t num_pipes;
  int i, j, k;
  bf_dev_port_t port;
  bf_tm_eg_q_t *_qdesc, *qdesc;
  bool result;

  LOG_TRACE("%s:", __func__);
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    for (k = 0; k < g_tm_ctx[dev]->tm_cfg.pg_per_pipe; k++) {
      port = MAKE_DEV_PORT(dev, k * g_tm_ctx[dev]->tm_cfg.ports_per_pg);
      _qdesc = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_restore_ctx[dev], port);
      qdesc = BF_TM_FIRST_Q_PTR_IN_PG(g_tm_ctx[dev], port);
      for (i = 0; i < g_tm_restore_ctx[dev]->tm_cfg.q_per_pg; i++) {
        sprintf(str, "Q-Cfg: Pipe = %d, Port = %d, Q = %d", j, k, i);
        BF_TM_CFG_VERIFY(str, "ppipe", _qdesc->p_pipe, qdesc->p_pipe);
        BF_TM_CFG_VERIFY(str, "lpipe", _qdesc->l_pipe, qdesc->l_pipe);
        BF_TM_CFG_VERIFY(str, "pg", _qdesc->pg, qdesc->pg);
        BF_TM_CFG_VERIFY(str, "port", _qdesc->port, qdesc->port);
        BF_TM_CFG_VERIFY(str, "logical_q", _qdesc->logical_q, qdesc->logical_q);
        BF_TM_CFG_VERIFY(
            str, "physical_q", _qdesc->physical_q, qdesc->physical_q);
        BF_TM_CFG_VERIFY(str,
                         "app_poolid",
                         _qdesc->q_cfg.app_poolid,
                         qdesc->q_cfg.app_poolid);
        BF_TM_CFG_VERIFY(str,
                         "isdynamic",
                         _qdesc->q_cfg.is_dynamic,
                         qdesc->q_cfg.is_dynamic);
        BF_TM_CFG_VERIFY(str, "baf", _qdesc->q_cfg.baf, qdesc->q_cfg.baf);
        BF_TM_CFG_VERIFY(str,
                         "tail_drop",
                         _qdesc->q_cfg.tail_drop_en,
                         qdesc->q_cfg.tail_drop_en);
        BF_TM_CFG_VERIFY(str,
                         "minlimit",
                         _qdesc->thresholds.min_limit,
                         qdesc->thresholds.min_limit);
        BF_TM_CFG_VERIFY(str,
                         "applimit",
                         _qdesc->thresholds.app_limit,
                         qdesc->thresholds.app_limit);
        BF_TM_CFG_VERIFY4(str,
                          "apphyst",
                          _qdesc->thresholds.app_hyst,
                          qdesc->thresholds.app_hyst);
        BF_TM_CFG_VERIFY(str,
                         "hyst index",
                         _qdesc->thresholds.app_hyst_index,
                         qdesc->thresholds.app_hyst_index);
        BF_TM_CFG_VERIFY(str,
                         "red_limit_pcent",
                         _qdesc->thresholds.red_limit_pcent,
                         qdesc->thresholds.red_limit_pcent);
        BF_TM_CFG_VERIFY(str,
                         "yel_limit_pcent",
                         _qdesc->thresholds.yel_limit_pcent,
                         qdesc->thresholds.yel_limit_pcent);
        BF_TM_CFG_VERIFY(str,
                         "red_hyst_index",
                         _qdesc->thresholds.red_hyst_index,
                         qdesc->thresholds.red_hyst_index);
        BF_TM_CFG_VERIFY(str,
                         "yel_hyst_index",
                         _qdesc->thresholds.yel_hyst_index,
                         qdesc->thresholds.yel_hyst_index);
        BF_TM_CFG_VERIFY(str,
                         "dwrr_wt",
                         _qdesc->q_sch_cfg.dwrr_wt,
                         qdesc->q_sch_cfg.dwrr_wt);
        BF_TM_CFG_VERIFY(str,
                         "colordrop",
                         _qdesc->q_cfg.color_drop_en,
                         qdesc->q_cfg.color_drop_en);

        BF_TM_CFG_VERIFY(str,
                         "fastrecovermode",
                         _qdesc->q_cfg.fast_recover_mode,
                         qdesc->q_cfg.fast_recover_mode);

        BF_TM_CFG_VERIFY4(str,
                          "red_hyst",
                          _qdesc->thresholds.red_hyst,
                          qdesc->thresholds.red_hyst);
        BF_TM_CFG_VERIFY4(str,
                          "yel_hyst",
                          _qdesc->thresholds.yel_hyst,
                          qdesc->thresholds.yel_hyst);
        BF_TM_CFG_VERIFY(str,
                         "sch_enabled",
                         _qdesc->q_sch_cfg.sch_enabled,
                         qdesc->q_sch_cfg.sch_enabled);
        BF_TM_CFG_VERIFY(str,
                         "sch_pfc_enabled",
                         _qdesc->q_sch_cfg.sch_pfc_enabled,
                         qdesc->q_sch_cfg.sch_pfc_enabled);
        BF_TM_CFG_VERIFY(
            str, "pps", _qdesc->q_sch_cfg.pps, qdesc->q_sch_cfg.pps);
        BF_TM_CFG_VERIFY(
            str, "cid", _qdesc->q_sch_cfg.cid, qdesc->q_sch_cfg.cid);
        BF_TM_CFG_VERIFY(str,
                         "pfc_sch_prio",
                         _qdesc->q_sch_cfg.pfc_prio,
                         qdesc->q_sch_cfg.pfc_prio);
        BF_TM_CFG_VERIFY(str,
                         "max_rate_sch_prio",
                         _qdesc->q_sch_cfg.max_rate_sch_prio,
                         qdesc->q_sch_cfg.max_rate_sch_prio);
        BF_TM_CFG_VERIFY(str,
                         "min_rate_sch_prio",
                         _qdesc->q_sch_cfg.min_rate_sch_prio,
                         qdesc->q_sch_cfg.min_rate_sch_prio);

        /*
         * Convert rates to mantissa-exponent form and verify
         * as both values may be different but after conversion it
         * should be same.
         */
        result = bf_tm_sch_cfg_verify_rate(dev,
                                           _qdesc->q_sch_cfg.min_rate,
                                           qdesc->q_sch_cfg.min_rate,
                                           qdesc->q_sch_cfg.pps);
        if (result == true) {
          BF_TM_CFG_VERIFY_PASS(str,
                                "minrate",
                                _qdesc->q_sch_cfg.min_rate,
                                qdesc->q_sch_cfg.min_rate);
        } else {
          BF_TM_CFG_VERIFY_FAIL(str,
                                "minrate",
                                _qdesc->q_sch_cfg.min_rate,
                                qdesc->q_sch_cfg.min_rate);
        }

        result = bf_tm_sch_cfg_verify_rate(dev,
                                           _qdesc->q_sch_cfg.max_rate,
                                           qdesc->q_sch_cfg.max_rate,
                                           qdesc->q_sch_cfg.pps);
        if (result == true) {
          BF_TM_CFG_VERIFY_PASS(str,
                                "maxrate",
                                _qdesc->q_sch_cfg.max_rate,
                                qdesc->q_sch_cfg.max_rate);
        } else {
          BF_TM_CFG_VERIFY_FAIL(str,
                                "maxrate",
                                _qdesc->q_sch_cfg.max_rate,
                                qdesc->q_sch_cfg.max_rate);
        }

        /*
         * Convert burst sizes to mantissa-exponent form and verify
         * as both values may be different but after conversion it
         * should be same.
         */
        result =
            bf_tm_sch_cfg_verify_burst_size(dev,
                                            _qdesc->q_sch_cfg.min_burst_size,
                                            qdesc->q_sch_cfg.min_burst_size);
        if (result == true) {
          BF_TM_CFG_VERIFY_PASS(str,
                                "min burstsize",
                                _qdesc->q_sch_cfg.min_burst_size,
                                qdesc->q_sch_cfg.min_burst_size);
        } else {
          BF_TM_CFG_VERIFY_FAIL(str,
                                "min burstsize",
                                _qdesc->q_sch_cfg.min_burst_size,
                                qdesc->q_sch_cfg.min_burst_size);
        }

        result =
            bf_tm_sch_cfg_verify_burst_size(dev,
                                            _qdesc->q_sch_cfg.max_burst_size,
                                            qdesc->q_sch_cfg.max_burst_size);
        if (result == true) {
          BF_TM_CFG_VERIFY_PASS(str,
                                "max burstsize",
                                _qdesc->q_sch_cfg.max_burst_size,
                                qdesc->q_sch_cfg.max_burst_size);
        } else {
          BF_TM_CFG_VERIFY_FAIL(str,
                                "max burstsize",
                                _qdesc->q_sch_cfg.max_burst_size,
                                qdesc->q_sch_cfg.max_burst_size);
        }
      }
    }
  }
  return (rc);
}

static bf_tm_status_t bf_tm_verify_pipe_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  char str[200];
  int j;
  uint32_t num_pipes;
  bf_tm_eg_pipe_t *p, *_p;

  LOG_TRACE("%s:", __func__);
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (j = 0; j < (int)num_pipes; j++) {
    sprintf(str, "Pipe-Cfg: Pipe = %d", j);
    _p = g_tm_restore_ctx[dev]->pipes + j;
    p = g_tm_ctx[dev]->pipes + j;
    BF_TM_CFG_VERIFY(str, "p_pipe", _p->p_pipe, p->p_pipe);
    BF_TM_CFG_VERIFY(str, "l_pipe", _p->l_pipe, p->l_pipe);
    BF_TM_CFG_VERIFY(str, "limit", _p->epipe_limit, p->epipe_limit);
    BF_TM_CFG_VERIFY(
        str, "resume_limit", _p->epipe_resume_limit, p->epipe_resume_limit);
    BF_TM_CFG_VERIFY(
        str, "ifg_comp", _p->ifg_compensation, p->ifg_compensation);
    if (p->neg_mirror_dest && _p->neg_mirror_dest) {
      // until default setting programs neg_mirror_port, its not possible
      // verify config. Check if negative mirror dest is programmed prior
      // to verifying with restored value.
      BF_TM_CFG_VERIFY(str,
                       "neg_mirror_ppipe",
                       _p->neg_mirror_dest->p_pipe,
                       p->neg_mirror_dest->p_pipe);
      BF_TM_CFG_VERIFY(str,
                       "neg_mirror_port",
                       _p->neg_mirror_dest->port,
                       p->neg_mirror_dest->port);
      BF_TM_CFG_VERIFY(str,
                       "neg_mirror_phy_q",
                       _p->neg_mirror_dest->physical_q,
                       p->neg_mirror_dest->physical_q);
    }
  }
  return (rc);
}

static bf_tm_status_t bf_tm_verify_mc_cfg(bf_dev_id_t dev) {
  bf_tm_status_t rc = BF_TM_EOK;
  LOG_TRACE("%s:", __func__);
  char str[200];
  bf_tm_mcast_t *fifo_ptr, *_fifo_ptr;

  uint32_t num_pipes = 0;
  lld_sku_get_num_active_pipes(dev, &num_pipes);
  for (int pipe = 0; pipe < (int)num_pipes; pipe++) {
    for (int fifo = 0; fifo < (int)g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe;
         fifo++) {
      sprintf(str, "Pipe-Cfg: Pipe = %d FIFO = %d", pipe, fifo);

      fifo_ptr = g_tm_ctx[dev]->mcast_fifo +
                 (pipe * g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe) + fifo;
      _fifo_ptr = g_tm_restore_ctx[dev]->mcast_fifo +
                  (pipe * g_tm_ctx[dev]->tm_cfg.pre_fifo_per_pipe) + fifo;

      BF_TM_CFG_VERIFY(str, "pipe", _fifo_ptr->l_pipe, fifo_ptr->l_pipe);
      BF_TM_CFG_VERIFY(str, "fifo", _fifo_ptr->fifo, fifo_ptr->fifo);
      BF_TM_CFG_VERIFY(
          str, "icos bmap", _fifo_ptr->icos_bmap, fifo_ptr->icos_bmap);
      BF_TM_CFG_VERIFY(
          str, "arb_mode", _fifo_ptr->arb_mode, fifo_ptr->arb_mode);
      BF_TM_CFG_VERIFY(str, "wrr_weight", _fifo_ptr->weight, fifo_ptr->weight);
      BF_TM_CFG_VERIFY(str, "depth", _fifo_ptr->size, fifo_ptr->size);
    }
  }

  return (rc);
}

typedef struct {
  bf_tm_status_t (*fpointer)(bf_dev_id_t);
  char descr[64];
  bool result;
} bf_tm_ut_hitl_tcase_t;

static bf_tm_status_t bf_tm_ut_verify_restored_cfg(
    bf_dev_id_t dev, bf_tm_status_t (*fpointer)(bf_dev_id_t)) {
  bf_tm_status_t rc = BF_TM_EOK;
  LOG_TRACE("%s:", __func__);
  if (fpointer) {
    rc = (fpointer)(dev);
  } else {
    rc = BF_TM_EINT;
  }
  return rc;
}

bf_tm_status_t bf_tm_ut_restore_device_cfg(ucli_context_t *uc,
                                           bf_dev_id_t dev) {
  bf_tm_asic_en asic_type = (bf_tm_asic_en)g_tm_ctx[dev]->asic_type;
  bf_tm_status_t rc = BF_TM_EOK;

  if (asic_type != BF_TM_ASIC_TOFINO) {
    aim_printf(&uc->pvs, "%s", "This functionality not supported yet.\n");
    return rc;
  }

  bf_tm_ut_hitl_tcase_t cases[TEST_CASE_COUNT] = {
      {&bf_tm_verify_ppg_cfg, "PPG cfg", false},
      {&bf_tm_verify_q_profile, "Q profile cfg", false},
      {&bf_tm_verify_port_cfg, "Port cfg", false},
      {&bf_tm_verify_q_cfg, "Q cfg", false},
      {&bf_tm_verify_pipe_cfg, "Pipe cfg", false},
      {&bf_tm_verify_mc_cfg, "MC cfg", false}};

  LOG_TRACE("Restoring TM cfg as UT for hitless HA case.");

  rc = bf_tm_init_restore_device(dev, asic_type);

  if (rc == BF_SUCCESS) {
    rc = bf_tm_restore_dev_cfg(dev);
    if (rc == BF_SUCCESS) {
      for (int i = 0; i < TEST_CASE_COUNT; ++i) {
        rc = bf_tm_ut_verify_restored_cfg(dev, cases[i].fpointer);
        cases[i].result = (rc == BF_TM_EOK);
      }
      for (int i = 0; i < TEST_CASE_COUNT; ++i) {
        aim_printf(&uc->pvs,
                   "%-20s%-10s",
                   cases[i].descr,
                   (cases[i].result) ? "........OK\n" : "......FAIL\n");
      }
      rc = BF_SUCCESS;
    } else {
      LOG_ERROR("Hitless HA TM cfg restoration failed.");
      aim_printf(&uc->pvs, "%s", "Can not restore device. FAILED\n");
    }
    // free up TM ctx memory.
    rc = tm_cleanup_device(g_tm_restore_ctx[dev]);
    g_tm_restore_ctx[dev] = NULL;
  } else {
    aim_printf(&uc->pvs, "%s", "Can not init restore TM. FAILED\n");
  }
  return (rc);
}
