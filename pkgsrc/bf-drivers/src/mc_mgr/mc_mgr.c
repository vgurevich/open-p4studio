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


#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <target-sys/bf_sal/bf_sys_dma.h>
#include <dvm/bf_drv_intf.h>
#include <dvm/bf_dma_types.h>
#include <lld/lld_sku.h>
#include <lld/bf_dma_if.h>
#include <lld/lld_dr_if.h>
#include <mc_mgr/mc_mgr_intf.h>
#include "mc_mgr.h"
#include "mc_mgr_handle.h"
#include "mc_mgr_bh.h"
#include "mc_mgr_ha.h"
#include <mc_mgr/mc_mgr_shared_dr.h>

static struct mc_mgr_ctx ctx;
struct mc_mgr_ctx *mc_mgr_ctx_p = NULL;

static bf_status_t mc_mgr_add_device(bf_dev_id_t dev,
                                     bf_dev_family_t dev_family,
                                     bf_device_profile_t *dont_care,
                                     bf_dma_info_t *dma_info,
                                     bf_dev_init_mode_t warm_init_mode);
static bf_status_t mc_mgr_rmv_device(bf_dev_id_t dev);
static bf_status_t mc_mgr_add_port(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   bf_port_attributes_t *port_attrib,
                                   bf_port_cb_direction_t direction);
static bf_status_t mc_mgr_rmv_port(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   bf_port_cb_direction_t direction);
static bf_status_t mc_mgr_warm_init_quick(bf_dev_id_t dev);
static bf_status_t mc_mgr_lock_device(bf_dev_id_t dev_id);
static bf_status_t mc_mgr_create_dma(bf_dev_id_t dev_id);
static bf_status_t mc_mgr_unlock_device(bf_dev_id_t dev_id);
static bf_status_t mc_mgr_config_complete(bf_dev_id_t dev_id);

bf_status_t mc_mgr_create_session(bf_mc_session_hdl_t *shdl) {
  bf_status_t ret;
  int i;

  ret = BF_MAX_SESSIONS_EXCEEDED;
  for (i = 0; i < MC_MGR_NUM_SESSIONS; i++) {
    if (mc_mgr_ctx_session_state(i) == NULL ||
        mc_mgr_ctx_session_state(i)->valid) {
      continue;
    }
    mc_mgr_ctx_session_state(i)->valid = true;
    mc_mgr_ctx_session_state(i)->shdl = mc_mgr_encode_sess_hdl(i);
    *shdl = mc_mgr_ctx_session_state(i)->shdl;
    ret = BF_SUCCESS;
    break;
  }

  return ret;
}

bf_status_t mc_mgr_init() {
  if (mc_mgr_ctx()) {
    return BF_SUCCESS;
  }

  /* Clear our context. */
  MC_MGR_MEMSET(&ctx, 0, sizeof(struct mc_mgr_ctx));

  /* Initialize all locks. */
  MC_MGR_LOCK_INIT_R(ctx.mtx_top);

  mc_mgr_ctx_p = &ctx;

  /* Initialize LLD interface. */
  bf_status_t sts = mc_mgr_drv_init();
  MC_MGR_ASSERT(BF_SUCCESS == sts);

  /* Register with DVM for add/remove device notifications. */
  bf_drv_client_handle_t bf_drv_hdl;
  bf_status_t r = bf_drv_register("mc-mgr", &bf_drv_hdl);
  MC_MGR_ASSERT(r == BF_SUCCESS);

  bf_drv_client_callbacks_t callbacks = {0};
  callbacks.device_add = mc_mgr_add_device;
  callbacks.device_del = mc_mgr_rmv_device;
  callbacks.port_add = mc_mgr_add_port;
  callbacks.port_del = mc_mgr_rmv_port;
  callbacks.lock = mc_mgr_lock_device;
  callbacks.create_dma = mc_mgr_create_dma;
  callbacks.unlock_reprogram_core = mc_mgr_unlock_device;
  callbacks.config_complete = mc_mgr_config_complete;
  callbacks.complete_hitless_hw_read = mc_mgr_hitless_ha_hw_read;
  callbacks.compute_delta_changes = mc_mgr_compute_delta_changes;
  callbacks.push_delta_changes = mc_mgr_push_delta_changes;
  callbacks.warm_init_quick = mc_mgr_warm_init_quick;
  bf_drv_client_register_callbacks(bf_drv_hdl, &callbacks, BF_CLIENT_PRIO_2);

  /* Get a pipe manager session. */
  pipe_status_t ps;
  pipe_sess_hdl_t shdl;
  ps = pipe_mgr_client_init(&shdl);
  if (PIPE_SUCCESS != ps) {
    LOG_ERROR("Failed to setup client session to pipe_mgr (%d)", ps);
    MC_MGR_ASSERT(PIPE_SUCCESS == ps);
    return BF_NOT_READY;
  }
  mc_mgr_ctx_pipe_sess_set(shdl);
  bf_mc_session_hdl_t int_sess;
  sts = mc_mgr_create_session(&int_sess);
  MC_MGR_ASSERT(BF_SUCCESS == sts);
  mc_mgr_ctx_int_sess_set(int_sess);

  return BF_SUCCESS;
}

static bf_status_t mc_mgr_add_device(bf_dev_id_t dev,
                                     bf_dev_family_t dev_family,
                                     bf_device_profile_t *dont_care,
                                     bf_dma_info_t *dma_info,
                                     bf_dev_init_mode_t warm_init_mode) {
  if (dev >= MC_MGR_NUM_DEVICES) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  if (mc_mgr_dev_present(dev)) {
    LOG_ERROR("%s: Device %d already present", __func__, dev);
    return BF_ALREADY_EXISTS;
  }
  (void)dont_care;
  bool syncing = false;
  int tvt_item_count_per_entry;
  int pvt_item_count_per_entry;
  int mc_mgr_2_die_factor = 1;
  int subdev = 0;

/* Used for programming 2 die in emulator and model, drivers running as 2
 * devices */
#if defined(EMU_2DIE_USING_SW_2DEV)
  mc_mgr_2_die_factor = 2;
#else
  mc_mgr_2_die_factor = 1;
#endif

  mc_mgr_init();

  if (BF_DEV_WARM_INIT_FAST_RECFG == warm_init_mode) {
    mc_mgr_ctx_dev_locked_set(dev, true);
  } else if (BF_DEV_WARM_INIT_HITLESS == warm_init_mode) {
    syncing = true;
  }

  struct mc_mgr_dev_ctx_t *d =
      MC_MGR_CALLOC(1, sizeof(struct mc_mgr_dev_ctx_t));
  if (!d) {
    LOG_ERROR("Failed to allocate memory for device %d", dev);
    return BF_NO_SYS_RESOURCES;
  }

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      d->pvt = MC_MGR_CALLOC(8 * 1024, sizeof(mc_mgr_pvt_entry_t));
      d->pvt_sz = 8 * 1024;
      pvt_item_count_per_entry = 8;
      tvt_item_count_per_entry = 0;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      d->pvt = MC_MGR_CALLOC(16 * 1024, sizeof(mc_mgr_pvt_entry_t));
      d->pvt_sz = 16 * 1024;
      pvt_item_count_per_entry = 4;
      tvt_item_count_per_entry = 0;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      d->pvt = MC_MGR_CALLOC(8 * 1024, sizeof(mc_mgr_pvt_entry_t));
      d->pvt_sz = 8 * 1024;
      pvt_item_count_per_entry = 8;
      tvt_item_count_per_entry = 4;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  if (!d->pvt) {
    MC_MGR_FREE(d);
    LOG_ERROR("Failed to allocate pvt memory for device %d", dev);
    return BF_NO_SYS_RESOURCES;
  }

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
      d->tvt = NULL;
      d->tvt_sz = 0;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      d->tvt = MC_MGR_CALLOC(16 * 1024, sizeof(mc_mgr_tvt_entry_t));
      d->tvt_sz = 16 * 1024;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return BF_INVALID_ARG;
  }
  if ((dev_family == BF_DEV_FAMILY_TOFINO3) && !d->tvt) {
    MC_MGR_FREE(d);
    LOG_ERROR("Failed to allocate tvt memory for device %d", dev);
    return BF_NO_SYS_RESOURCES;
  }

  d->dev_family = dev_family;
  lld_sku_get_num_subdev(dev, &(d->num_subdevices), NULL);
  d->num_max_pipes =
      BF_SUBDEV_PIPE_COUNT * d->num_subdevices * mc_mgr_2_die_factor;
  MC_MGR_ASSERT(d->num_max_pipes <= MC_MGR_NUM_PIPES);

  d->num_max_ports =
      MC_MGR_SUBDEV_PORT_COUNT * d->num_subdevices * mc_mgr_2_die_factor;
  MC_MGR_ASSERT(d->num_max_ports <= MC_MGR_PORT_COUNT);

  d->syncing = syncing;
  d->ecmp_hw_id_gen = bf_id_allocator_new(0x0000FFFF, false);
  d->ecmp_sw_id_gen = bf_id_allocator_new(0x00FFFFFF, false);
  d->l1_node_id_gen = bf_id_allocator_new(0x00FFFFFF, false);

  for (subdev = 0; subdev < (int)d->num_subdevices; ++subdev) {
    MC_MGR_LOCK_INIT(d->tree_len_mtx[subdev]);
  }

  d->pmt_size = MC_MGR_SUBDEV_PORT_COUNT;

  int i;
  int j;
  /* Initialize the PMT shadow. */
  for (i = 0; i < (int)d->num_max_ports; ++i)
    bf_bs_init(&d->pmt[i], MC_MGR_PMT_SIZE, d->pmt_[i]);

  /* Initialize the LIT shadow. */
  for (i = 0; i < BF_LAG_COUNT; ++i) {
    for (j = 0; j < MC_MGR_NUM_PIPES; ++j) {
      bf_bs_init(&d->lit[i][j], BF_PIPE_PORT_COUNT, d->lit_[i][j]);
    }
  }
  /* Initialize RDM shadow. */
  mc_mgr_rdm_map_init(&d->rdm_map, dev, dev_family, d->num_max_pipes);
  /* Initialize Tree-Sizes. */
  for (subdev = 0; subdev < (int)d->num_subdevices; ++subdev) {
    bf_map_init(&d->tree_len[subdev]);
  }
  for (int p = 0; p < (int)d->num_max_pipes; ++p) {
    for (subdev = 0; subdev < (int)d->num_subdevices; ++subdev) {
      bf_map_init(&d->trees_with_len_updated[subdev][p]);
    }
  }

  for (int ses = 0; ses < MC_MGR_NUM_SESSIONS; ++ses) {
    bf_map_init(&d->rdm_addrs_to_free[ses]);
  }

  mc_mgr_ctx_dev_set(dev, d);

  bf_status_t sts = mc_mgr_drv_init_dev(dev, dma_info);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  int sid = 0;
  if (d->dev_family == BF_DEV_FAMILY_TOFINO) {
    /* Allocate a block for node tails. */
    struct mc_mgr_tail_info *ti = mc_mgr_ctx_tail_info(dev);
    ti->tail_l2_size = 260;
    uint16_t l2_sizes[] = {3,  4,  5,  6,  7,   8,   9,   10,  12,  14, 16,
                           18, 20, 22, 24, 26,  28,  30,  32,  36,  40, 44,
                           48, 56, 64, 72, 100, 125, 150, 175, 200, 260};
    ti->num_tails = sizeof l2_sizes / sizeof l2_sizes[0];
    ti->tail_size = MC_MGR_CALLOC(ti->num_tails, sizeof *ti->tail_size);
    MC_MGR_MEMCPY(ti->tail_size, l2_sizes, sizeof l2_sizes);
    for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      sts = mc_mgr_rdm_reserve_tails(sid, dev, p);
      if (BF_SUCCESS != sts) return sts;
    }
  }

  /* Set default max nodes.  4k L1 nodes, 128 LAG nodes, 5 port nodes. */
  mc_mgr_set_max_nodes(sid, dev, 4096, 128 + 5);

  /* Set default max L1 time slice. */
  mc_mgr_set_l1_time_slice(sid, dev, 16);

  /* Initialize backup port table. */
  uint16_t x;
  for (x = 0; x < mc_mgr_ctx_num_max_ports(dev); ++x) {
    mc_mgr_ctx_bkup_port_set(dev, x, x);
    mc_mgr_set_bkup_port_wrl(sid, dev, 0, x, x);
    mc_mgr_set_bkup_port_wrl(sid, dev, 1, x, x);
  }

  /* Initialize MIT. */
  int pipe, mgid;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    for (mgid = 0; mgid < BF_MGID_COUNT; mgid += 4) {
      sts = mc_mgr_set_mit_wrl(sid, dev, pipe, mgid);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s: Error initializing MIT, dev %d pipe %d sts %s",
                  __func__,
                  dev,
                  pipe,
                  bf_err_str(sts));
        return sts;
      }
    }
  }

  /* Initialize PVT */
  for (uint32_t r = 0; r < mc_mgr_ctx_pvt_sz(dev); ++r) {
    for (i = 0; i < pvt_item_count_per_entry; i++) {
      sts = mc_mgr_program_pvt(sid,
                               dev,
                               r * pvt_item_count_per_entry + i,
                               0,
                               MC_PVT_MASK_ALL,
                               true,
                               __func__,
                               __LINE__);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s: Error initializing PVT, dev %d base %d sts %s",
                  __func__,
                  dev,
                  r,
                  bf_err_str(sts));
        return sts;
      }
    }
  }
  if (!mc_mgr_ctx_dev_locked(dev)) {
    pipe_mgr_mc_pipe_msk_update_push(mc_mgr_ctx_pipe_sess(), true);
  }

  /* Initialize TVT */
  for (uint32_t r = 0; r < mc_mgr_ctx_tvt_sz(dev); ++r) {
    for (i = 0; i < tvt_item_count_per_entry; i++) {
      sts = mc_mgr_program_tvt(sid,
                               dev,
                               r * tvt_item_count_per_entry + i,
                               0x1,
                               true,
                               __func__,
                               __LINE__);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s: Error initializing tvt, dev %d base %d sts %s",
                  __func__,
                  dev,
                  r,
                  bf_err_str(sts));
        return sts;
      }
    }
  }
  if (!mc_mgr_ctx_dev_locked(dev)) {
    for (uint32_t r = 0; r < mc_mgr_ctx_pvt_sz(dev); ++r) {
      sts = mc_mgr_pvt_write_row_from_shadow(
          dev, sid, BF_DEV_PIPE_ALL, r, MC_PVT_MASK_ALL, true);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s: Error initializing PVT, dev %d row %d sts %s",
                  __func__,
                  dev,
                  r,
                  bf_err_str(sts));
        return sts;
      }
    }
    pipe_mgr_mc_pipe_msk_update_push(mc_mgr_ctx_pipe_sess(), true);
  }

  /* Initialize LAG and LAG-NP. */
  for (int ver = 0; ver < 2; ++ver) {
    for (int lag_id = 0; lag_id < BF_LAG_COUNT; ++lag_id) {
      (void)mc_mgr_set_lit_wrl(sid, dev, ver, lag_id);
    }
    for (int lag_id = 0; lag_id < BF_LAG_COUNT; ++lag_id) {
      (void)mc_mgr_set_lit_np_wrl(sid, dev, ver, lag_id);
    }
  }

  /* Initialize the LAG pipe mask and lag2node map. */
  for (int lag_id = 0; lag_id < BF_LAG_COUNT; ++lag_id) {
    d->lag2pipe_mask[lag_id] = 0;
    bf_map_init(&d->lag2node_map[lag_id]);
  }

  /* Initiailze PMT. */
  int yid;
  int ver;
  for (yid = 0; yid < (int)(d->pmt_size); ++yid) {
    for (ver = 0; ver < 2; ++ver) {
      mc_mgr_set_pmt_wrl(sid, dev, ver, yid);
    }
  }
  bf_map_init(&d->mgrpinfo_map);
  (void)mc_mgr_drv_wrl_send(sid, true);

  LOG_TRACE("%s: Device %d add successful.", __func__, dev);
  return BF_SUCCESS;
}

static bf_status_t mc_mgr_rmv_device(bf_dev_id_t dev) {
  int pipe = 0;
  bf_map_sts_t s, s1;
  mc_l1_node_t *node = NULL;
  mc_ecmp_grp_t *node_ecmp = NULL;
  struct mc_mgr_grp_info *mgrp_info = NULL;
  unsigned long not_used;
  int subdev = 0;

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_dev_present(dev)) {
    mc_mgr_one_at_a_time_end();
    return BF_SUCCESS;
  }

  LOG_TRACE("%s: Device %d remove.", __func__, dev);

  if (dev >= MC_MGR_NUM_DEVICES) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* Iterate through group info map and clean up the group state. */
  while (BF_MAP_OK == bf_map_get_first_rmv(mc_mgr_ctx_mgrp_info_map(dev),
                                           &not_used,
                                           (void *)&mgrp_info)) {
    if (!mgrp_info) continue;
    bf_map_t *mbrs_map = &mgrp_info->node_mbrs;

    /* Clean up the nodes associated to the group members map. */
    do {
      s = bf_map_get_first_rmv(mbrs_map, &not_used, (void *)&node);
    } while (s == BF_MAP_OK);

    bf_map_destroy(mbrs_map);
    MC_MGR_FREE(mgrp_info);
  }

  /* Destroy the mgroup info map. */
  bf_map_destroy(mc_mgr_ctx_mgrp_info_map(dev));

  /* Cleanup L1 nodes for device */
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;) {
    if ((!node) || (node && (node->dev != dev))) {
      s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
      continue;
    }
    for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      node->hw_nodes[pipe].rdm_addr = 0;
    }
    mc_mgr_node_free(dev, node->handle);
    /* Get the First node again as nodes might have got re-arranged after delete
     */
    s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
  }

  /* Cleanup ecmp-grps for device */
  for (s = bf_map_get_first(
           mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&node_ecmp);
       BF_MAP_OK == s;) {
    if ((!node_ecmp) || (node_ecmp && (node_ecmp->dev != dev))) {
      s = bf_map_get_next(
          mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&node_ecmp);
      continue;
    }

    /* Remove ECMP group from DB. */
    s1 = bf_map_rmv(mc_mgr_ctx_db_ecmp(dev), node_ecmp->handle);
    MC_MGR_DBGCHK(BF_MAP_OK == s1);
    mc_mgr_delete_ecmp_hdl(dev, node_ecmp->handle);
    MC_MGR_FREE(node_ecmp);
    /* Get the First node again as nodes might have got re-arranged after delete
     */
    s = bf_map_get_first(
        mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&node_ecmp);
  }

  /* CLean the bf_map of MGID to reference count for each LAG indexes. */
  for (int lag_id = 0; lag_id < BF_LAG_COUNT; lag_id++) {
    uint32_t *data_ref;
    do {
      s = bf_map_get_first_rmv(mc_mgr_ctx_lag_to_node_map(dev, lag_id),
                               &not_used,
                               (void *)&data_ref);
    } while (s == BF_MAP_OK);
    bf_map_destroy(mc_mgr_ctx_lag_to_node_map(dev, lag_id));
  }

  mc_mgr_rdm_map_cleanup(&mc_mgr_ctx_dev(dev)->rdm_map);

  /* Clean up DMA pool. */
  mc_mgr_drv_remove_dev(dev);

  bf_id_allocator_destroy(mc_mgr_ctx_ecmp_hw_id_gen(dev));
  bf_id_allocator_destroy(mc_mgr_ctx_ecmp_sw_id_gen(dev));
  bf_id_allocator_destroy(mc_mgr_ctx_l1_id_gen(dev));

  if (mc_mgr_ctx_dev(dev)->pvt) MC_MGR_FREE(mc_mgr_ctx_dev(dev)->pvt);

  /* Clean up tree sizes. */
  for (subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev); ++subdev) {
    mc_mgr_ctx_tree_len_rmv_all(dev, subdev);
    bf_map_destroy(mc_mgr_ctx_tree_len(dev, subdev));
  }
  struct mc_mgr_tail_info *ti = mc_mgr_ctx_tail_info(dev);
  if (ti && ti->tail_size) MC_MGR_FREE(ti->tail_size);

  for (subdev = 0; subdev < (int)mc_mgr_ctx_num_subdevices(dev); ++subdev) {
    mc_mgr_ctx_tree_len_lock(dev, subdev);
    for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      while (-1 != mc_mgr_ctx_get_rmv_len_update(0, dev, subdev, p)) {
      }
      bf_map_destroy(mc_mgr_ctx_trees_with_len_updated(dev, subdev, p));
    }
    mc_mgr_ctx_tree_len_unlock(dev, subdev);

    MC_MGR_LOCK_DEL(mc_mgr_ctx_dev(dev)->tree_len_mtx[subdev]);
  }

  for (int ses = 0; ses < MC_MGR_NUM_SESSIONS; ++ses) {
    bf_map_destroy(mc_mgr_ctx_rdm_free_addrs(ses, dev));
  }

  /* Free the port-list */
  while (mc_mgr_ctx_dev(dev)->port_list) {
    mc_mgr_port_info_t *port_info = mc_mgr_ctx_dev(dev)->port_list;
    BF_LIST_DLL_REM(mc_mgr_ctx_dev(dev)->port_list, port_info, next, prev);
    MC_MGR_FREE(port_info);
  }

  MC_MGR_FREE(mc_mgr_ctx_dev(dev));
  mc_mgr_ctx_dev_set(dev, NULL);

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

mc_mgr_port_info_t *mc_mgr_get_port_info(bf_dev_id_t dev, uint16_t port_id) {
  mc_mgr_port_info_t *port_info;

  if (!mc_mgr_dev_present(dev)) {
    MC_MGR_DBGCHK(0);
    return NULL;
  }

  for (port_info = mc_mgr_ctx_dev(dev)->port_list; port_info;
       port_info = port_info->next) {
    if (port_info && port_info->port_id == port_id) {
      return port_info;
    }
  }

  return NULL;
}

bf_status_t mc_mgr_update_l1_node_port_speed(bf_dev_id_t dev,
                                             bf_dev_port_t port_id,
                                             bf_port_speeds_t speed,
                                             bool port_add) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_l1_node_t *node = NULL;
  unsigned long not_used;
  (void)port_add;

  /* Need to update speed only for tofino3 */
  if (mc_mgr_ctx_dev_family(dev) != BF_DEV_FAMILY_TOFINO3) {
    return BF_SUCCESS;
  }

  /* We only care of 400G ports */
  if (speed != BF_SPEED_400G) {
    return BF_SUCCESS;
  }

  bf_mc_session_hdl_t shdl = mc_mgr_ctx_int_sess();
  int sid = mc_mgr_validate_session(shdl, __func__, __LINE__);

  /* Go over all L1 nodes to find this port */
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (mc_mgr_l1_node_contains_port(node, port_id)) {
      /* Push PVT config again */
      uint8_t pvt = mc_mgr_get_pvt(dev, node->mgid);
      sts = mc_mgr_program_pvt(sid,
                               dev,
                               node->mgid,
                               pvt,
                               MC_PVT_MASK_ALL,
                               false,
                               __func__,
                               __LINE__);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("%s: Error programming PVT, dev %d sts %s",
                  __func__,
                  dev,
                  bf_err_str(sts));
        return sts;
      }
    }
  }

  return BF_SUCCESS;
}

static bf_status_t mc_mgr_add_port(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   bf_port_attributes_t *port_attrib,
                                   bf_port_cb_direction_t direction) {
  bf_port_speeds_t speed = port_attrib->port_speeds;
  bf_status_t sts = BF_SUCCESS;

  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_dev_present(dev)) {
    sts = BF_INVALID_ARG;
    goto end;
  }

  if (dev >= MC_MGR_NUM_DEVICES) {
    sts = BF_INVALID_ARG;
    goto end;
  }

  port_id = lld_sku_map_devport_from_user_to_device(dev, port_id);

  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    mc_mgr_port_info_t *port_info;

    LOG_TRACE("%s: Dev %d, Port %d add.", __func__, dev, port_id);

    /* Check if a port already exists with the port_id */
    if ((port_info = mc_mgr_get_port_info(dev, port_id))) {
      LOG_ERROR("%s: port with port_id(%d) already exists", __func__, port_id);
      sts = BF_ALREADY_EXISTS;
      goto end;
    }

    port_info = MC_MGR_MALLOC(sizeof(mc_mgr_port_info_t));
    if (port_info == NULL) {
      sts = BF_NO_SYS_RESOURCES;
      goto end;
    }
    MC_MGR_MEMSET(port_info, 0, sizeof(mc_mgr_port_info_t));

    /* Initialize port ID and speed */
    port_info->port_id = port_id;
    port_info->speed = speed;

    /* Add to the list of ports on the device */
    BF_LIST_DLL_AP(mc_mgr_ctx_dev(dev)->port_list, port_info, next, prev);

    /* Set port speed on L1 node */
    mc_mgr_update_l1_node_port_speed(dev, port_id, speed, true);
  }

end:
  mc_mgr_one_at_a_time_end();

  return sts;
}

static bf_status_t mc_mgr_rmv_port(bf_dev_id_t dev,
                                   bf_dev_port_t port_id,
                                   bf_port_cb_direction_t direction) {
  mc_mgr_port_info_t *port_info;
  mc_mgr_one_at_a_time_begin();

  if (!mc_mgr_dev_present(dev)) {
    mc_mgr_one_at_a_time_end();
    return BF_INVALID_ARG;
  }

  if (dev >= MC_MGR_NUM_DEVICES) {
    mc_mgr_one_at_a_time_end();
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  port_id = lld_sku_map_devport_from_user_to_device(dev, port_id);

  if (direction == BF_PORT_CB_DIRECTION_EGRESS) {
    LOG_TRACE("%s: Dev %d, Port %d remove.", __func__, dev, port_id);
    /* Find the port */
    if ((port_info = mc_mgr_get_port_info(dev, port_id)) == NULL) {
      LOG_TRACE("%s: port with port_id(%d) not found", __func__, port_id);
      mc_mgr_one_at_a_time_end();
      return BF_SUCCESS;
    }
    bf_port_speeds_t speed = port_info->speed;

    BF_LIST_DLL_REM(mc_mgr_ctx_dev(dev)->port_list, port_info, next, prev);

    /* Free port object */
    MC_MGR_FREE(port_info);

    /* Set port speed on L1 node */
    mc_mgr_update_l1_node_port_speed(dev, port_id, speed, false);
  }

  mc_mgr_one_at_a_time_end();

  return BF_SUCCESS;
}

static bf_status_t mc_mgr_warm_init_quick(bf_dev_id_t dev) {
  /* Cleanup trees_with_len_updated */
  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev); subdev++) {
    mc_mgr_ctx_tree_len_lock(dev, subdev);
    for (int pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      int mgid = 0;
      while (-1 != mgid) {
        mgid = mc_mgr_ctx_get_rmv_len_update(0, dev, subdev, pipe);
      }
    }
    mc_mgr_ctx_tree_len_unlock(dev, subdev);
  }

  /* Cleanup rdm_addrs_to_free */
  for (int ses = 0; ses < MC_MGR_NUM_SESSIONS; ++ses) {
    bf_map_t *rdm_map = mc_mgr_ctx_rdm_free_addrs(ses, dev);
    unsigned long key = 0;
    void *data = NULL;
    bf_map_sts_t map_status = BF_MAP_OK;
    while (BF_MAP_OK == map_status) {
      map_status = bf_map_get_first_rmv(rdm_map, &key, &data);
    }
  }

  /* Zero DMA pool. */
  mc_mgr_drv_warm_init_quick(dev);

  return BF_SUCCESS;
}

bf_status_t mc_mgr_mgrp_create(bf_dev_id_t dev, bf_mc_grp_id_t mgid) {
  struct mc_mgr_tree_size_t *t = NULL;

  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev); subdev++) {
    t = MC_MGR_CALLOC(mc_mgr_ctx_num_max_pipes(dev), sizeof *t);
    if (!t) {
      return BF_NO_SYS_RESOURCES;
    }
    bf_status_t s = mc_mgr_ctx_tree_len_add(dev, subdev, mgid, t);
    if (BF_SUCCESS != s) {
      MC_MGR_FREE(t);
      MC_MGR_DBGCHK(BF_SUCCESS == s);
      return s;
    }
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_mgrp_destroy(int sid, bf_dev_id_t dev, bf_mc_grp_id_t mgid) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_l1_node_t *node = NULL;
  mc_ecmp_grp_t *ecmp = NULL;
  unsigned long not_used;
  /* Find all ECMP groups associated with this MGID and remove their
   * association.  */
  for (s = bf_map_get_first(mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&ecmp);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&ecmp)) {
    for (mc_l1_node_t *n = ecmp->refs; n; n = n->ecmp_next) {
      if (mgid == n->mgid) {
        sts = mc_mgr_ecmp_dissociate(sid, dev, mgid, ecmp);
        if (BF_SUCCESS != sts) {
          LOG_ERROR("Failed to remove ECMP 0x%x from MGID 0x%x on dev %d",
                    ecmp->handle,
                    mgid,
                    dev);
          return sts;
        }
        break;
      }
    }
  }
  /* Find all nodes associated with this MGID and remove their association. */
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (mgid == node->mgid) {
      sts = mc_mgr_l1_dissociate(sid, node, mgid);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to remove node 0x%x from MGID 0x%x on dev %d",
                  node->handle,
                  mgid,
                  dev);
        return sts;
      }
    }
  }
  /* Clean up the tree length memory. */
  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev); subdev++) {
    mc_mgr_ctx_tree_len_rmv(dev, subdev, mgid);
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_mgrp_get_first_node_mbr(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           bf_mc_node_hdl_t *node_hdl,
                                           bool *node_l1_xid_valid,
                                           bf_mc_l1_xid_t *node_l1_xid) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_l1_node_t *node = NULL;
  unsigned long not_used;
  struct mc_mgr_grp_info *mgrp_info = NULL;
  (void)sid;

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_WARN("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
             mgid,
             dev,
             s);
    return BF_OBJECT_NOT_FOUND;
  }

  MC_MGR_DBGCHK(mgrp_info->grp_id == mgid);

  bf_map_t *mbrs_map = &mgrp_info->node_mbrs;

  /* Traverse all nodes and get the first node for the given mgid */
  for (s = bf_map_get_first(mbrs_map, &not_used, (void *)&node); BF_MAP_OK == s;
       s = bf_map_get_next(mbrs_map, &not_used, (void *)&node)) {
    if (!node->ecmp_grp) {
      MC_MGR_DBGCHK(mgid == node->mgid);
      *node_hdl = node->handle;
      *node_l1_xid_valid = node->xid_valid;
      *node_l1_xid = node->xid;
      /* Got the first node, return */
      return sts;
    }
  }

  *node_hdl = -1;  // invalid handle
  *node_l1_xid_valid = false;
  *node_l1_xid = 0;

  return BF_OBJECT_NOT_FOUND;
}

bf_status_t mc_mgr_mgrp_get_node_mbr_count(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           uint32_t *count) {
  (void)sid;

  /* The caller must check whether the group is already present. */
  MC_MGR_DBGCHK(mc_mgr_get_mgid_map_bit(dev, mgid) == true);

  struct mc_mgr_grp_info *mgrp_info = NULL;
  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  bf_status_t s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  *count = mc_mgr_mgrp_l1_count(dev, mgrp_info);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_mgrp_get_next_i_node_mbr(int sid,
                                            bf_dev_id_t dev,
                                            bf_mc_grp_id_t mgid,
                                            bf_mc_node_hdl_t node_hdl,
                                            uint32_t i,
                                            bf_mc_node_hdl_t *next_node_hdls,
                                            bool *next_node_l1_xids_valid,
                                            bf_mc_l1_xid_t *next_node_l1_xids) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_l1_node_t *node = NULL;
  unsigned long not_used;
  struct mc_mgr_grp_info *mgrp_info = NULL;
  (void)sid;

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_WARN("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
             mgid,
             dev,
             s);
    return BF_OBJECT_NOT_FOUND;
  }

  bf_map_t *mbrs_map = &mgrp_info->node_mbrs;

  /*
   * Traverse all the nodes and get next i nodes after the given node
   * in the multicast group.
   */
  bool search_mode = true;
  uint32_t cur = 0;
  for (s = bf_map_get_first(mbrs_map, &not_used, (void *)&node); BF_MAP_OK == s;
       s = bf_map_get_next(mbrs_map, &not_used, (void *)&node)) {
    if (node->ecmp_grp) {
      continue;
    }

    if (search_mode) {
      if (node_hdl == node->handle) {
        search_mode = false;
      }
      continue;
    }

    next_node_hdls[cur] = node->handle;
    next_node_l1_xids_valid[cur] = node->xid_valid;
    next_node_l1_xids[cur] = node->xid;
    cur++;

    if (cur == i) {
      break;
    }
  }

  /* If no node exists in the mgrp after the given node, return error */
  if (cur == 0) {
    return BF_OBJECT_NOT_FOUND;
  }

  /* If all i nodes are not filled, set invalid handles to remaining slots */
  for (; cur < i; cur++) {
    next_node_hdls[cur] = -1;  // invalid handle
    next_node_l1_xids_valid[cur] = 0;
    next_node_l1_xids[cur] = 0;
  }

  return sts;
}

bf_status_t mc_mgr_mgrp_get_first_ecmp_mbr(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           bf_mc_ecmp_hdl_t *ecmp_hdl,
                                           bool *ecmp_l1_xid_valid,
                                           bf_mc_l1_xid_t *ecmp_l1_xid) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_ecmp_grp_t *ecmp = NULL;
  unsigned long not_used;
  (void)sid;

  /* Traverse all the ECMPS and get the first ECMP member for the given mgid */
  for (s = bf_map_get_first(mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&ecmp);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&ecmp)) {
    for (mc_l1_node_t *n = ecmp->refs; n; n = n->ecmp_next) {
      if (mgid == n->mgid) {
        *ecmp_hdl = ecmp->handle;
        *ecmp_l1_xid_valid = n->xid_valid;
        *ecmp_l1_xid = n->xid;
        /* Got the first ecmp, return */
        return sts;
      }
    }
  }

  *ecmp_hdl = -1;  // invalid handle
  *ecmp_l1_xid_valid = false;
  *ecmp_l1_xid = 0;

  return BF_OBJECT_NOT_FOUND;
}

bf_status_t mc_mgr_mgrp_get_ecmp_mbr_count(int sid,
                                           bf_dev_id_t dev,
                                           bf_mc_grp_id_t mgid,
                                           uint32_t *count) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  (void)sid;

  *count = 0;

  /* The caller must check whether the group is already present. */
  MC_MGR_DBGCHK(mc_mgr_get_mgid_map_bit(dev, mgid) == true);

  struct mc_mgr_grp_info *mgrp_info = NULL;
  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  *count = mc_mgr_mgrp_ecmp_l1_count(dev, mgrp_info);
  return sts;
}

bf_status_t mc_mgr_mgrp_get_next_i_ecmp_mbr(int sid,
                                            bf_dev_id_t dev,
                                            bf_mc_grp_id_t mgid,
                                            bf_mc_ecmp_hdl_t ecmp_hdl,
                                            uint32_t i,
                                            bf_mc_ecmp_hdl_t *next_ecmp_hdls,
                                            bool *next_ecmp_l1_xids_valid,
                                            bf_mc_l1_xid_t *next_ecmp_l1_xids) {
  bf_status_t sts = BF_SUCCESS;
  bf_map_sts_t s;
  mc_ecmp_grp_t *ecmp = NULL;
  unsigned long not_used;
  (void)sid;

  /*
   * Traverse all the ECMPs and get next i ECMPs after the given ECMP
   * in the multicast group.
   */
  bool search_mode = true;
  uint32_t cur = 0;
  for (s = bf_map_get_first(mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&ecmp);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_ecmp(dev), &not_used, (void *)&ecmp)) {
    if (search_mode) {
      if (ecmp_hdl == ecmp->handle) {
        search_mode = false;
      }
      continue;
    }

    /* For ECMPs after the given ECMP, check if it's part of the given MGID */
    for (mc_l1_node_t *n = ecmp->refs; n; n = n->ecmp_next) {
      if (mgid != n->mgid) {
        continue;
      }

      /* MGID matches, add this ECMP and break to process next ECMP */
      next_ecmp_hdls[cur] = ecmp->handle;
      next_ecmp_l1_xids_valid[cur] = n->xid_valid;
      next_ecmp_l1_xids[cur] = n->xid;
      cur++;
      break;
    }

    /* If all i ECMPs are found, no need to process rest of ECMPs */
    if (cur == i) {
      break;
    }
  }

  /* If no ECMP exists in the mgrp after the given ECMP, return error */
  if (cur == 0) {
    return BF_OBJECT_NOT_FOUND;
  }

  /* If all i ECMPs are not filled, set invalid handles to remaining slots */
  for (; cur < i; cur++) {
    next_ecmp_hdls[cur] = -1;  // invalid handle
    next_ecmp_l1_xids_valid[cur] = 0;
    next_ecmp_l1_xids[cur] = 0;
  }

  return sts;
}

bf_status_t mc_mgr_ecmp_alloc(int sid, bf_dev_id_t dev, bf_mc_ecmp_hdl_t h) {
  bf_status_t sts = BF_SUCCESS;

  /* Decode the ECMP handle. */
  sts = mc_mgr_decode_ecmp_hdl(h, __func__, __LINE__);
  if (BF_SUCCESS != sts) {
    return BF_INVALID_ARG;
  }

  /* Allocate memory for the group object. */
  mc_ecmp_grp_t *grp = MC_MGR_MALLOC(sizeof(mc_ecmp_grp_t));
  if (!grp) {
    LOG_ERROR("Failed to allocate memory for ECMP group");
    return BF_NO_SYS_RESOURCES;
  }
  MC_MGR_MEMSET(grp, 0, sizeof(mc_ecmp_grp_t));

  int id = bf_id_allocator_allocate(mc_mgr_ctx_ecmp_hw_id_gen(dev));
  if (-1 == id) {
    LOG_ERROR("Failed to allocate HW Id for ECMP group on dev %d", dev);
    MC_MGR_FREE(grp);
    return BF_NO_SYS_RESOURCES;
  }

  /* Initialize fields. */
  grp->handle = h;
  grp->dev = dev;
  grp->ecmp_id = id;
  /* Insert into DB. */
  bf_map_sts_t s = bf_map_add(mc_mgr_ctx_db_ecmp(dev), h, grp);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  /* Allocate the vector nodes for the group. */
  int pipe = 0, ver = 0;
  for (ver = 0; ver < 2; ++ver) {
    for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      uint32_t a;
      a = mc_mgr_rdm_map_get(sid, dev, pipe, mc_mgr_rdm_node_type_vector, 1);
      if (!a) {
        goto cleanup1;
      }
      mc_mgr_rdm_write_vec(sid, dev, a, 0, 0, id);
      grp->vector_node_addr[ver][pipe] = a;
    }
  }

  return sts;
cleanup1:
  for (ver = 0; ver < 2; ++ver) {
    for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
      if (grp->vector_node_addr[ver][pipe]) {
        mc_mgr_rdm_map_return(dev, grp->vector_node_addr[ver][pipe]);
        grp->vector_node_addr[ver][pipe] = 0;
      }
    }
  }
  bf_map_rmv(mc_mgr_ctx_db_ecmp(dev), h);
  bf_id_allocator_release(mc_mgr_ctx_ecmp_hw_id_gen(dev), id);
  MC_MGR_FREE(grp);
  sts = BF_NO_SYS_RESOURCES;
  return sts;
}

bf_status_t mc_mgr_ecmp_free(int sid, bf_dev_id_t dev, mc_ecmp_grp_t *g) {
  bf_status_t sts = BF_SUCCESS;

  /* If the group is associated with any multicast groups clean those up. */
  while (g->refs) {
    int mgid = g->refs->mgid;
    sts = mc_mgr_ecmp_dissociate(sid, dev, mgid, g);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to remove ECMP group 0x%x from MGID 0x%x on dev %d, sts %s",
          g->handle,
          mgid,
          dev,
          bf_err_str(sts));
      return sts;
    }
  }
  /* If the group has any members remove them. */
  while (g->valid_map) {
    /* Find any member. */
    bf_mc_node_hdl_t mbr_hdl = 0;
    for (unsigned int i = 0; i < sizeof(g->mbrs) / sizeof(g->mbrs[0]); ++i) {
      if (g->mbrs[i]) {
        mbr_hdl = g->mbrs[i]->handle;
        break;
      }
    }
    if (0 == mbr_hdl) {
      LOG_ERROR("Failed to remove all members from ECMP group 0x%x on dev %d",
                g->handle,
                dev);
      return BF_UNEXPECTED;
    }
    mc_l1_node_t *mbr_node =
        mc_mgr_lookup_l1_node(dev, mbr_hdl, __func__, __LINE__);
    if (!mbr_node) {
      LOG_ERROR("Failed to remove node 0x%x from ECMP group 0x%x on dev %d",
                mbr_hdl,
                g->handle,
                dev);
      return BF_UNEXPECTED;
    }
    sts = mc_mgr_ecmp_mbr_rem(sid, dev, g, mbr_node);
    if (BF_SUCCESS != sts) {
      LOG_ERROR(
          "Failed to remove node 0x%x from ECMP group 0x%x on dev %d, sts %s",
          mbr_hdl,
          g->handle,
          dev,
          bf_err_str(sts));
      return sts;
    }
  }
  /* Remove vector nodes from HW. */
  int ver, pipe;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev); ++pipe) {
    for (ver = 0; ver < 2; ++ver) {
      if (g->vector_node_addr[ver][pipe]) {
        mc_mgr_rdm_map_enqueue_free(sid, dev, g->vector_node_addr[ver][pipe]);
        g->vector_node_addr[ver][pipe] = 0;
      }
    }
  }
  /* Remove ECMP group from DB. */
  bf_map_sts_t s = bf_map_rmv(mc_mgr_ctx_db_ecmp(dev), g->handle);
  MC_MGR_DBGCHK(BF_MAP_OK == s);
  /* Free the id. */
  mc_mgr_delete_ecmp_hdl(dev, g->handle);
  /* Free the HW id */
  bf_id_allocator_release(mc_mgr_ctx_ecmp_hw_id_gen(dev), g->ecmp_id);
  /* Free group. */
  MC_MGR_FREE(g);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_ecmp_mbr_add(int sid,
                                bf_dev_id_t dev,
                                mc_ecmp_grp_t *g,
                                mc_l1_node_t *n) {
  bf_status_t sts = BF_SUCCESS;
  if (g == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (n == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (!~g->valid_map) {
    LOG_ERROR("ECMP group %#x already has max members, cannot add node %#x",
              g->handle,
              n->handle);
    return BF_INVALID_ARG;
  }
  if (n->ecmp_grp == g) {
    LOG_ERROR(
        "Node %#x is already a member of ECMP group %#x", n->handle, g->handle);
    return BF_INVALID_ARG;
  }
  if (n->ecmp_grp) {
    LOG_ERROR("Node %#x is already a member of ECMP group %#x",
              n->handle,
              n->ecmp_grp->handle);
    return BF_INVALID_ARG;
  }
  if (-1 != n->mgid) {
    LOG_ERROR("Node %#x is already a member of multicast group %#x",
              n->handle,
              n->mgid);
    return BF_INVALID_ARG;
  }

  /* There are three cases below:
   * 1) The group is empty and it has not been programmed anywhere. It
   *    will be programmed to some set of pipes.
   * 2) The group is already programmed in some set of pipes and now it
   *    may be programmed for the first time some pipes while being
   *    modified in other pipes. The group has members, and it has more
   *    space allocated for the membership block than it actually uses.
   *    This couldhappen if a member had been removed and the group was
   *    not re-compacted.
   * 3) The group is already programmed in some set of pipes and now it
   *    may be programmed for the first time in some pipe while being
   *    modified in other pipes. The group has members and is using all
   *    the space allocated to it. The membership block is reallocated
   *    in existing and required pipes to accomadate the new member.
   */
  if (!g->allocated_sz) {
    return mc_mgr_ecmp_first_mbr_add(sid, dev, g, n);
  } else {
    /* Check for any holes in the existing allocation. */
    int x;
    for (x = 0; x < g->allocated_sz && g->mbrs[x]; ++x)
      ;
    if (x != g->allocated_sz) {
      return mc_mgr_ecmp_mbr_add_to_hole(sid, dev, g, n);
    } else {
      return mc_mgr_ecmp_mbr_add_to_end(sid, dev, g, n);
    }
  }

  return sts;
}

bf_status_t mc_mgr_ecmp_mbr_mod(int sid,
                                bf_dev_id_t dev,
                                mc_ecmp_grp_t *g,
                                mc_l1_node_t **ns,
                                uint32_t size) {
  bf_status_t sts = BF_SUCCESS;
  if (g == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (ns == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (size > MC_ECMP_GROUP_MAX_MBR) {
    LOG_ERROR(
        "Cannot hold more than %d mbrs in one ecmp group, current allocated "
        "%d, but request is to have total %u",
        MC_ECMP_GROUP_MAX_MBR,
        g->allocated_sz,
        size);
    return BF_INVALID_ARG;
  }

  // ecmp_grp can only have max 32 members
  mc_l1_node_t *add_list[MC_ECMP_GROUP_MAX_MBR] = {0};
  mc_l1_node_t *del_list[MC_ECMP_GROUP_MAX_MBR] = {0};
  uint32_t add_count = 0;
  uint32_t del_count = 0;
  // A - B => refer to venn diagram A-B
  // get difference to be added
  for (uint32_t i = 0; i < size; i++) {
    bool old_list_has_it = false;
    for (int j = 0; j < MC_ECMP_GROUP_MAX_MBR; j++) {
      bool valid = g->valid_map & (1u << j);
      if (valid) {
        if (g->mbrs[j] == ns[i]) {
          old_list_has_it = true;
          break;
        }
      } else {
        continue;
      }
    }
    if (old_list_has_it == false) {
      add_list[add_count] = ns[i];
      add_count++;
    }
  }
  // get difference to be deleted
  for (int i = 0; i < MC_ECMP_GROUP_MAX_MBR; i++) {
    bool new_list_has_it = false;
    bool valid = g->valid_map & (1u << i);
    if (valid) {
      for (uint32_t j = 0; j < size; j++) {
        if (ns[j] == g->mbrs[i]) {
          new_list_has_it = true;
          break;
        }
      }
      if (new_list_has_it == false) {
        del_list[del_count] = g->mbrs[i];
        del_count++;
      }
    }
  }
  // TODO various verifications (currently implemented in single add/remove
  // fucntion)
  // 1. ecmp_grp has enough size
  // 2. every node has not been assigned with a mgid
  // 3. (maybe) need to drain the session with given nodes
  for (uint32_t i = 0; i < add_count; i++) {
    mc_l1_node_t *n = add_list[i];
    if (mgid_associated(n)) {
      LOG_ERROR("Node %#x is already a member of multicast group %#x",
                n->handle,
                n->mgid);
      return BF_INVALID_ARG;
    }
    if (n->ecmp_grp == g) {
      LOG_WARN("Node %#x is already a member of ECMP group %#x",
               n->handle,
               g->handle);
      MC_MGR_DBGCHK(false);
    } else if (n->ecmp_grp) {
      LOG_ERROR("Node %#x is already a member of ECMP group %#x",
                n->handle,
                n->ecmp_grp->handle);
      return BF_INVALID_ARG;
    }
  }
  // delete them
  for (uint32_t i = 0; i < del_count; i++) {
    mc_l1_node_t *n = del_list[i];
    sts = mc_mgr_ecmp_mbr_rem(sid, dev, g, n);
    if (sts != BF_SUCCESS) {
      LOG_ERROR(
          "mc_mgr bulk mods cannot remove node %#x from group %#x, it is a "
          "member of group %#x",
          n->handle,
          g->handle,
          n->ecmp_grp->handle);
      MC_MGR_DBGCHK(false);
      return BF_UNEXPECTED;
    }
  }
  // add the list
  uint32_t progress = 0;
  for (uint32_t i = 0; i < add_count; i++) {
    mc_l1_node_t *n = add_list[i];
    sts = mc_mgr_ecmp_mbr_add(sid, dev, g, n);
    if (sts != BF_SUCCESS) {
      LOG_ERROR(
          "mc_mgr bulk mods cannot add node %#x from group %#x, it is a member "
          "of group %#x",
          n->handle,
          g->handle,
          n->ecmp_grp->handle);
      break;
    } else {
      progress = i + 1;
    }
  }
  if (progress != add_count) {
    // rollback previous add
    for (uint32_t i = 0; i < progress; i++) {
      mc_l1_node_t *n = del_list[i];
      sts = mc_mgr_ecmp_mbr_rem(sid, dev, g, n);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "mc_mgr bulk mods cannot roll back added node %#x from group %#x, "
            "it is a member of group %#x",
            n->handle,
            g->handle,
            n->ecmp_grp->handle);
        MC_MGR_DBGCHK(false);
      }
    }
    // rollback previous del
    for (uint32_t i = 0; i < del_count; i++) {
      mc_l1_node_t *n = del_list[i];
      sts = mc_mgr_ecmp_mbr_add(sid, dev, g, n);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "mc_mgr bulk mods cannot rollback deleted node %#x from group %#x, "
            "it is a member of group %#x",
            n->handle,
            g->handle,
            n->ecmp_grp->handle);
        MC_MGR_DBGCHK(false);
      }
    }
    return BF_NO_SYS_RESOURCES;
  }
  return sts;
}

bf_status_t mc_mgr_ecmp_mbr_rem(int sid,
                                bf_dev_id_t dev,
                                mc_ecmp_grp_t *g,
                                mc_l1_node_t *n) {
  if (g == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (n == NULL) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  if (n->ecmp_grp == NULL) {
    LOG_ERROR(
        "Cannot remove node %#x from group %#x - not a member of this group",
        n->handle,
        g->handle);
    return BF_OBJECT_NOT_FOUND;
  }

  if (n->ecmp_grp != g) {
    LOG_ERROR(
        "Cannot remove node %#x from group %#x, it is a member of group %#x",
        n->handle,
        g->handle,
        n->ecmp_grp->handle);
    return BF_INVALID_ARG;
  }

  bf_status_t sts;
  sts = mc_mgr_ecmp_mbr_rem_one_mbr(sid, dev, g, n);
  LOG_DBG(
      "Removed mbr_hdl:%#x from grp_hdl:%#x dev:%d", n->handle, g->handle, dev);
  return sts;
}

bf_status_t mc_mgr_l1_associate(int sid,
                                mc_l1_node_t *node,
                                bf_mc_grp_id_t mgid,
                                bf_dev_id_t dev,
                                bf_mc_l1_xid_t xid,
                                bool use_xid) {
  bf_status_t sts = BF_SUCCESS;
  struct mc_mgr_grp_info *mgrp_info = NULL;

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  bf_map_sts_t s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_ERROR("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
              mgid,
              dev,
              s);
    return BF_OBJECT_NOT_FOUND;
  }

  if (-1 != node->mgid) {
    LOG_ERROR(
        "Failed to associated L1 node %#x with group %#x, already "
        "associated with group %#x",
        node->handle,
        mgid,
        node->mgid);
    return BF_ALREADY_EXISTS;
  } else if (node->ecmp_grp) {
    LOG_ERROR(
        "Failed to associated L1 node %#x with group %#x, already "
        "associated with ecmp group %#x",
        node->handle,
        mgid,
        node->ecmp_grp->handle);
    return BF_ALREADY_EXISTS;
  }

  node->mgid = mgid;
  node->dev = dev;
  node->xid = use_xid ? xid : 0;
  node->xid_valid = use_xid;

  sts = mc_mgr_l1_write(sid, node);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to write node 0x%x in group 0x%x on dev %d",
              node->handle,
              mgid,
              dev);
  } else {
    /* As the write has been successfull, add the node to the
     * group mbrs map. */
    bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
    s = bf_map_add(mbrs_map, node->handle, node);
    MC_MGR_DBGCHK(BF_MAP_OK == s);

    mc_mgr_mgrp_inc_l1_count(dev, mgrp_info);
  }
  return sts;
}

bf_status_t mc_mgr_l1_dissociate(int sid,
                                 mc_l1_node_t *node,
                                 bf_mc_grp_id_t mgid) {
  struct mc_mgr_grp_info *mgrp_info = NULL;

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(node->dev);
  bf_map_sts_t s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_ERROR("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
              mgid,
              node->dev,
              s);
    return BF_OBJECT_NOT_FOUND;
  }

  if (-1 == node->mgid) {
    LOG_ERROR(
        "Failed to dissocaited L1 node %#x from group %#x, already "
        "dissocaited",
        node->handle,
        mgid);
    return BF_ALREADY_EXISTS;
  } else if (mgid != node->mgid) {
    LOG_ERROR(
        "Failed to dissocaited L1 node %#x from group %#x, "
        "associated with group %#x",
        node->handle,
        mgid,
        node->mgid);
    return BF_ALREADY_EXISTS;
  } else if (node->ecmp_grp) {
    LOG_ERROR(
        "Failed to associated L1 node %#x with group %#x, already "
        "associated with ecmp group %#x",
        node->handle,
        mgid,
        node->ecmp_grp->handle);
    return BF_ALREADY_EXISTS;
  }

  /* First remove the member from the group map. */
  bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
  s = bf_map_rmv(mbrs_map, node->handle);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  /* Decrement the member count. */
  mc_mgr_mgrp_dec_l1_count(node->dev, mgrp_info);

  bf_status_t sts = mc_mgr_l1_remove(sid, node);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to remove node 0x%x to group 0x%x mapping on dev %d",
              node->handle,
              mgid,
              node->dev);

    MC_MGR_DBGCHK(0);
  }
  return sts;
}

bf_status_t mc_mgr_ecmp_associate(int sid,
                                  bf_dev_id_t dev,
                                  bf_mc_grp_id_t mgid,
                                  mc_ecmp_grp_t *ecmp,
                                  bf_mc_l1_xid_t xid,
                                  bool use_xid) {
  bf_status_t sts = BF_SUCCESS;
  struct mc_mgr_grp_info *mgrp_info = NULL;

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  bf_map_sts_t s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_ERROR("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
              mgid,
              dev,
              s);
    return BF_OBJECT_NOT_FOUND;
  }

  /* Ensure ecmp group isn't already assocaited to this mgid. */
  mc_l1_node_t *n;
  for (n = ecmp->refs; n; n = n->ecmp_next) {
    if (n->mgid == mgid) {
      LOG_ERROR("ECMP group %#x is already associated with group %#x",
                ecmp->handle,
                mgid);
      return BF_ALREADY_EXISTS;
    }
  }

  /* Allocate an L1 node. */
  n = mc_mgr_node_alloc(dev, 0xDEAD);
  if (!n) return BF_NO_SYS_RESOURCES;
  n->mgid = mgid;
  n->xid = xid;
  n->xid_valid = use_xid;
  n->ecmp_grp = ecmp;

  /* Write it in all pipes (also updates MIT and PVT). */
  sts = mc_mgr_ecmp_ptr_write(sid, n);
  if (BF_SUCCESS == sts) {
    /* Link it onto the group. */
    BF_LIST_DLL_PP(ecmp->refs, n, ecmp_next, ecmp_prev);

    /* Add the L1 node to the group members map. */
    bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
    s = bf_map_add(mbrs_map, n->handle, n);
    MC_MGR_DBGCHK(BF_MAP_OK == s);
    mc_mgr_mgrp_ecmp_inc_l1_count(dev, mgrp_info);
  } else {
    mc_mgr_node_free(n->dev, n->handle);
  }

  return sts;
}

bf_status_t mc_mgr_ecmp_dissociate(int sid,
                                   bf_dev_id_t dev,
                                   bf_mc_grp_id_t mgid,
                                   mc_ecmp_grp_t *ecmp) {
  bf_status_t sts = BF_SUCCESS;
  struct mc_mgr_grp_info *mgrp_info = NULL;

  bf_map_t *grpinfo_map = mc_mgr_ctx_mgrp_info_map(dev);
  bf_map_sts_t s = bf_map_get(grpinfo_map, mgid, (void **)&mgrp_info);
  if (s != BF_MAP_OK || !mgrp_info) {
    LOG_WARN("Failed to get mgrp obj with group %#x on dev %d map_sts:%d",
             mgid,
             dev,
             s);
    return BF_OBJECT_NOT_FOUND;
  }

  /* Ensure ecmp group is already assocaited to this mgid. */
  mc_l1_node_t *n;
  for (n = ecmp->refs; n; n = n->ecmp_next) {
    if (n->mgid == mgid) {
      break;
    }
  }
  if (!n) {
    LOG_ERROR(
        "ECMP group %#x is not associated with mgid %#x", ecmp->handle, mgid);
    return BF_INVALID_ARG;
  }
  if (n->ecmp_grp != ecmp) {
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  /* Remove the L1 ptr from the members map. */
  bf_map_t *mbrs_map = &mgrp_info->node_mbrs;
  s = bf_map_rmv(mbrs_map, n->handle);
  MC_MGR_DBGCHK(BF_MAP_OK == s);

  mc_mgr_mgrp_ecmp_dec_l1_count(dev, mgrp_info);

  /* Unlink it from the group. */
  BF_LIST_DLL_REM(ecmp->refs, n, ecmp_next, ecmp_prev);

  /* Free it in all pipes (also updates MIT and PVT). */
  sts = mc_mgr_ecmp_ptr_remove(sid, n);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  /* Free the L1 node. */
  sts = mc_mgr_node_free(dev, n->handle);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  /* Get the size of this tree now that the ECMP group has been removed and
   * update the tail if needed.  Note that we need to defer updating the tail
   * in hardware until an RDM change signal is received so we know that no
   * packets are walking the (longest) removed node. */
  for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    int l2_len = mc_mgr_compute_longest_l2(dev, p, mgid);
    mc_mgr_update_len(sid, dev, p, mgid, l2_len);
  }

  return sts;
}

bf_status_t mc_mgr_lag_update(int sid,
                              bf_dev_id_t dev,
                              int lag_id,
                              bf_mc_port_map_t port_map) {
  bf_status_t sts = BF_SUCCESS;

  bf_bitset_t lag_old_bset[BF_PIPE_COUNT];
  uint64_t oldlag[BF_PIPE_COUNT][BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];

  /* Save the original old lag bset. */
  for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); p++) {
    bf_bs_init(&lag_old_bset[p], BF_PIPE_PORT_COUNT, &oldlag[p][0]);
    bf_bs_copy(&lag_old_bset[p], mc_mgr_ctx_lit(dev, lag_id, p));
  }

  /* We are iterating twice for the lag update in order to achieve hitless.
   * Itr-0: Will push the lags with old_port_map | new_port_map. Hence, running
   * traffic before HW LIT update will not have any drop. Post LIT update the
   * traffic will start using the new map. Iter-1: Will push the lags with
   * new_port_map and remove the old map. As the LIT is already updated with new
   * map, there is no impact on the running traffic using the LIT. */
  int itr = 0;
  while (itr < 2) {
    for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); p++) {
      bf_bitset_t lag_bset;
      uint64_t lag[BF_BITSET_ARRAY_SIZE(BF_PIPE_PORT_COUNT)];
      bf_bs_init(&lag_bset, BF_PIPE_PORT_COUNT, lag);

      /* As the port map is presented as bit vectors, the old and new port
       * map can be "OR"'ed. */
      int i;
      int step = BF_PIPE_PORT_COUNT / 8;
      for (i = 0; i < step; ++i) {
        uint8_t old_word = bf_bs_get_word(&lag_old_bset[p], 8 * i, 8);
        uint8_t tgt_word = 0;
        if (itr == 0) {
          tgt_word = port_map[i + p * step] | old_word;
        } else {
          tgt_word = port_map[i + p * step];
        }
        bf_bs_set_word(&lag_bset, 8 * i, 8, tgt_word);
      }

      /* Update shadow. */
      bf_bs_copy(mc_mgr_ctx_lit(dev, lag_id, p), &lag_bset);
    }
    /* Program backup table. */
    int cur_tbl_ver = mc_mgr_ctx_tbl_ver(dev);
    sts = mc_mgr_set_lit_wrl(sid, dev, !cur_tbl_ver, lag_id);
    if (sts != BF_SUCCESS) {
      LOG_ERROR("Failed to program LIT, dev %d ver %d lag_id %d",
                dev,
                !cur_tbl_ver,
                lag_id);
      for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); p++) {
        bf_bs_copy(mc_mgr_ctx_lit(dev, lag_id, p), &lag_old_bset[p]);
      }
      return sts;
    }

    int old_lag_pm = mc_mgr_ctx_lag2pipe_mask(dev, lag_id);
    int new_lag_pm = mc_mgr_calculate_lag2pipe_mask(dev, lag_id);

    LOG_TRACE("Dev:%d lag_id:%d old_lag_pm:%x new_lag_pm:%x",
              dev,
              lag_id,
              old_lag_pm,
              new_lag_pm);

    if (old_lag_pm != new_lag_pm) {
      /* Iterate through the lag to MGID map and find all L1/L1-End nodes using
       * this lag_id. Internally we call mc_mgr_l1_write() for L1/L1-End nodes
       * and alter L2 lag nodes according to the new lag pipe-mask and  update
       * the TVP/PVT internally. */
      mc_mgr_ctx_lag2pipe_mask_set(dev, lag_id, new_lag_pm);
      sts = mc_mgr_update_lag_to_l1_nodes(sid, dev, lag_id);
      if (sts != BF_SUCCESS) {
        LOG_ERROR(
            "Failed to update lag-to-nodes, dev %d old_mask:%x new_mask:%x "
            "lag_id %d iter:%d",
            dev,
            old_lag_pm,
            new_lag_pm,
            lag_id,
            itr);

        /* We can fail in 2nd iteration. In that scenario, as we have
         * already updated the LIT with the (new_port_map | old_port_map)
         * as part of the first iteration, we don't need to reset the
         * LIT with old_port_map. Move ahead and update the HW LIT
         * table with new port map and change it version. Presence of
         *  lags nodes in old pipes will not see the traffic, as traffic
         * never gets hashed to them because the HW LIT got updated with
         * new port map. */
        if (itr == 0) {
          for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); p++) {
            bf_bs_copy(mc_mgr_ctx_lit(dev, lag_id, p), &lag_old_bset[p]);
          }
          MC_MGR_DBGCHK(BF_SUCCESS == sts);
          return sts;
        }
      }
    }

    /* Perform versioning switch if needed. */
    if (mc_mgr_versioning_on(sid, dev)) {
      if (BF_SUCCESS != (sts = mc_mgr_drv_wrl_send(sid, false))) {
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        return sts;
      }
      mc_mgr_drv_cmplt_operations(sid, dev);

      sts = mc_mgr_program_tbl_ver(sid, dev, !cur_tbl_ver);
      if (BF_SUCCESS != sts) {
        MC_MGR_DBGCHK(0);
        return sts;
      }

      /* No need to update PVT since LAGs are in all pipes. */

      /* Wait for old version to drain. */
      sts = mc_mgr_wait_for_ver_drain(dev, cur_tbl_ver);
      if (BF_SUCCESS != sts) {
        MC_MGR_DBGCHK(0);
        return sts;
      }
    }

    /* Program other version. */
    sts = mc_mgr_set_lit_wrl(sid, dev, cur_tbl_ver, lag_id);
    if (sts != BF_SUCCESS) {
      LOG_ERROR("Failed to program LIT, dev %d ver %d lag_id %d",
                dev,
                cur_tbl_ver,
                lag_id);
      mc_mgr_drv_wrl_abort(sid);
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      return sts;
    }

    itr++;
  }
  return sts;
}

bf_status_t mc_mgr_lag_update_rmt_cnt(
    int sid, bf_dev_id_t dev, int lag_id, int l_cnt, int r_cnt) {
  bf_status_t sts = BF_SUCCESS;

  int cur_tbl_ver = mc_mgr_ctx_tbl_ver(dev);
  uint16_t l_old = mc_mgr_ctx_lit_np_l(dev, lag_id);
  uint16_t r_old = mc_mgr_ctx_lit_np_r(dev, lag_id);

  /* Program backup LAG-NP. */
  mc_mgr_ctx_lit_np_l_set(dev, lag_id, l_cnt);
  mc_mgr_ctx_lit_np_r_set(dev, lag_id, r_cnt);
  sts = mc_mgr_set_lit_np_wrl(sid, dev, !cur_tbl_ver, lag_id);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program LIT-NP, dev %d ver %d lag_id %d",
              dev,
              !cur_tbl_ver,
              lag_id);
    mc_mgr_ctx_lit_np_l_set(dev, lag_id, l_old);
    mc_mgr_ctx_lit_np_r_set(dev, lag_id, r_old);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }
  if (mc_mgr_versioning_on(sid, dev)) {
    if (BF_SUCCESS != (sts = mc_mgr_drv_wrl_send(sid, false))) {
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      return sts;
    }
    mc_mgr_drv_cmplt_operations(sid, dev);

    /* Switch table versions. */
    sts = mc_mgr_program_tbl_ver(sid, dev, !cur_tbl_ver);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(0);
      return sts;
    }

    /* Wait for old version to drain. */
    sts = mc_mgr_wait_for_ver_drain(dev, cur_tbl_ver);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(0);
      return sts;
    }
  }

  /* Program "new backup" LAG-NP table. */
  sts = mc_mgr_set_lit_np_wrl(sid, dev, cur_tbl_ver, lag_id);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program LIT-NP, dev %d ver %d lag_id %d",
              dev,
              cur_tbl_ver,
              lag_id);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  return sts;
}

bf_status_t mc_mgr_get_port_fwd_state(int sid,
                                      bf_dev_id_t dev,
                                      int port_bit_index,
                                      bool *is_active) {
  (void)sid;
  mc_mgr_ctx_port_fwd_state_get_one(dev, port_bit_index, is_active);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_port_fwd_state(int sid,
                                      bf_dev_id_t dev,
                                      int port,
                                      bool inactive) {
  bf_status_t sts = BF_SUCCESS;
  int cur_tbl_ver = mc_mgr_ctx_tbl_ver(dev);

  /* Program backup port mask in the software shadow. */
  mc_mgr_ctx_port_fwd_state_set_one(dev, port, inactive);
  /* Queue up the change on a DMA write list. */
  sts = mc_mgr_set_port_mask_wrl(sid, dev, !cur_tbl_ver, port);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program port mask, dev %d ver %d port %d",
              dev,
              !cur_tbl_ver,
              port);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  if (mc_mgr_versioning_on(sid, dev)) {
    /* Push the operations to hardware. */
    sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to send write list to set port mask dev %d sts %s",
                dev,
                bf_err_str(sts));
      return sts;
    }
    /* Switch table versions. */
    sts = mc_mgr_program_tbl_ver(sid, dev, !cur_tbl_ver);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(0);
      return sts;
    }

    /* Wait for old version to drain. */
    sts = mc_mgr_wait_for_ver_drain(dev, cur_tbl_ver);
    if (BF_SUCCESS != sts) {
      MC_MGR_DBGCHK(0);
      return sts;
    }
  }

  /* Program "new backup" port mask table. */
  sts = mc_mgr_set_port_mask_wrl(sid, dev, cur_tbl_ver, port);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program port mask, dev %d ver %d port %d",
              dev,
              cur_tbl_ver,
              port);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  return sts;
}

bf_status_t mc_mgr_set_port_ff_mode(int sid, bf_dev_id_t dev, bool en) {
  bool old = mc_mgr_ctx_ff_en(dev);
  mc_mgr_ctx_ff_en_set(dev, en);

  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_set_comm_ctrl_wrl(sid, dev);
  if (BF_SUCCESS != sts) {
    mc_mgr_ctx_ff_en_set(dev, old);
    mc_mgr_drv_wrl_abort(sid);
  }
  return sts;
}

bf_status_t mc_mgr_clr_port_ff_state(bf_dev_id_t dev, int port) {
  return mc_mgr_clr_port_down_reg(dev, port);
}

bf_status_t mc_mgr_get_port_ff_state(bf_dev_id_t dev,
                                     int port_bit_index,
                                     bool *is_active) {
  bool is_down = false;
  bf_status_t sts = mc_mgr_get_port_down_reg(dev, port_bit_index, &is_down);
  if (sts != BF_SUCCESS) return sts;
  *is_active = !is_down;
  return sts;
}

bf_status_t mc_mgr_set_backup_port_mode(int sid, bf_dev_id_t dev, bool en) {
  bool old = mc_mgr_ctx_bkup_port_en(dev);
  mc_mgr_ctx_bkup_port_en_set(dev, en);

  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_set_comm_ctrl_wrl(sid, dev);
  if (BF_SUCCESS != sts) {
    mc_mgr_ctx_bkup_port_en_set(dev, old);
    mc_mgr_drv_wrl_abort(sid);
  }
  return sts;
}

bf_status_t mc_mgr_get_backup_port(int sid,
                                   bf_dev_id_t dev,
                                   int protected_bit_idx,
                                   int *backup_port_bit_idx) {
  (void)sid;
  *backup_port_bit_idx = mc_mgr_ctx_bkup_port(dev, protected_bit_idx);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_backup_port(int sid,
                                   bf_dev_id_t dev,
                                   int pbit_idx,
                                   int bbit_idx) {
  bf_status_t sts = BF_SUCCESS;
  uint32_t addr = 0;
  int p;

  mc_mgr_rdm_addr_list_t *to_clean = NULL;
  mc_mgr_rdm_addr_list_t *to_write = NULL;
  mc_mgr_rdm_addr_list_t *to_write_lags = NULL;
  mc_l1_node_t *node = NULL;
  unsigned long not_used;

  /* Protected port pipe. */
  int ppipe = mc_bit_idx_to_pipe(dev, pbit_idx);
  int plocal_port = mc_bit_idx_to_local_port(dev, pbit_idx);
  if (ppipe < 0 || ppipe >= (int)mc_mgr_ctx_num_max_pipes(dev)) {
    return BF_SUCCESS;
  }

  // Update shadow.
  int old_backup = mc_mgr_ctx_bkup_port(dev, pbit_idx);
  mc_mgr_ctx_bkup_port_set(dev, pbit_idx, bbit_idx);

  /* Old and New Mask List. */
  int *old_lag_pm = MC_MGR_CALLOC(BF_LAG_COUNT, sizeof(int));
  if (!old_lag_pm) {
    LOG_ERROR("Failed to allocate old_mask tbl dev:%x", dev);
    mc_mgr_ctx_bkup_port_set(dev, pbit_idx, old_backup);
    return BF_NO_SYS_RESOURCES;
  }

  int *new_lag_pm = MC_MGR_CALLOC(BF_LAG_COUNT, sizeof(int));
  if (!new_lag_pm) {
    LOG_ERROR("Failed to allocate new_mask tbl dev:%x", dev);
    MC_MGR_FREE(old_lag_pm);
    mc_mgr_ctx_bkup_port_set(dev, pbit_idx, old_backup);
    return BF_NO_SYS_RESOURCES;
  }

  /* Find LAGs which are used by the protected port. */
  bf_bitset_t prot_port_lags;
  uint64_t lags_arr[BF_BITSET_ARRAY_SIZE(BF_LAG_COUNT)];
  bf_bs_init(&prot_port_lags, BF_LAG_COUNT, lags_arr);
  int lag_id;
  for (lag_id = 0; lag_id < BF_LAG_COUNT; lag_id++) {
    if (bf_bs_get(mc_mgr_ctx_lit(dev, lag_id, ppipe), plocal_port)) {
      bf_bs_set(&prot_port_lags, lag_id, 1);
    }
  }

  /* Fill up the old mask table. */
  lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&prot_port_lags, lag_id))) {
    old_lag_pm[lag_id] = mc_mgr_ctx_lag2pipe_mask(dev, lag_id);
  }

  /* Fill up the new mask table. */
  lag_id = -1;
  while (-1 != (lag_id = bf_bs_first_set(&prot_port_lags, lag_id))) {
    new_lag_pm[lag_id] = mc_mgr_calculate_lag2pipe_mask(dev, lag_id);
    mc_mgr_ctx_lag2pipe_mask_set(dev, lag_id, new_lag_pm[lag_id]);
  }

  /* Handle L1-End nodes whose LAG map has changed and are
   * associated with MGID. */
  sts = mc_mgr_set_backup_ecmps(sid,
                                dev,
                                old_lag_pm,
                                new_lag_pm,
                                pbit_idx,
                                bbit_idx,
                                old_backup,
                                &prot_port_lags);
  if (sts != BF_SUCCESS) goto cleanup;

  /* Walk L1 nodes and count number of L1 and L2 nodes to alloc. */
  bf_map_sts_t s;
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (node == NULL) {
      MC_MGR_DBGCHK(0);
      return BF_UNEXPECTED;
    }

    /* Skip L1-ECMP nodes or any ECMP member nodes. They are
     * handled separately.
     */
    if (node->ecmp_grp) continue;

    /* Skip L1 nodes which are not using the protected port as they will
     * not change. */
    bf_bitset_t ports;
    bf_dev_pipe_t pipe = mc_bit_idx_to_pipe(dev, pbit_idx);
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[pipe]);

    /* Consider only nodes where the protected port is a member of the node.
     * Also check if the protected port is a member of node's lags.*/
    if (!bf_bs_get(&ports, mc_bit_idx_to_local_port(dev, pbit_idx)) &&
        !mc_mgr_is_prot_port_node_lags(
            dev, node, mc_bit_idx_to_dev_port(dev, pbit_idx)))
      continue;

    /* Skip L1RID/XID nodes which are not associated as they have no HW
     * resources  yet. */
    if (-1 == node->mgid) continue;
    /* Calculate the pipe mask where L1-RID nodes need to be allocated. */
    int l1_add_mask =
        mc_mgr_calculate_backup_l1_add_mask(node, old_backup, bbit_idx);
    /* Calculate pipe mask which includes old and new backup pipe mask. */
    int backup_mod_pm =
        mc_mgr_calculate_backup_mod_pm(node, old_backup, bbit_idx);
    LOG_DBG(
        "L1 mod_pm:%x add_pm:%x port_bit:%d old_backup_p:%d new_backup_p:%d",
        backup_mod_pm,
        l1_add_mask,
        pbit_idx,
        old_backup,
        bbit_idx);

    /* Iterate through pipes and handle pipes where new ports/lags
     * need to be allocated. */
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      if (!(backup_mod_pm & (1 << p))) continue;

      /* Check if an L1 needs to be added to this pipe. */
      if ((l1_add_mask & (1 << p))) {
        uint32_t new_l1_rdm_addr =
            mc_mgr_rdm_map_get(sid, dev, p, mc_mgr_rdm_node_type_rid, 1);
        if (!new_l1_rdm_addr) {
          sts = BF_NO_SYS_RESOURCES;
          goto cleanup;
        }
        mc_mgr_rdm_addr_append(&to_write, new_l1_rdm_addr);
      }

      /* Assume the port node will be blindly re-written, therefore
       * allocate a block of RDM nodes to hold the new port node(s). If the
       * block size is not zero then only allocate port-node. */
      int block_sz;
      mc_mgr_get_port_node_size(dev, p, node, &block_sz, NULL);
      if (block_sz) {
        uint32_t new_l2_rdm_addr = mc_mgr_rdm_map_get(
            sid, dev, p, mc_mgr_rdm_node_type_port72, block_sz);
        if (!new_l2_rdm_addr) {
          sts = BF_NO_SYS_RESOURCES;
          goto cleanup;
        }
        mc_mgr_rdm_addr_append(&to_write, new_l2_rdm_addr);
      }

      lag_id = -1;
      bf_bitset_t node_lags;
      bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);

      /* Check if we have any LAG nodes to be allocated in this pipe. */
      while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
        /* Check if the LAG id is being used by the protected port */
        if (!bf_bs_get(&prot_port_lags, lag_id)) continue;
        if (!((new_lag_pm[lag_id] & (1 << p)) &&
              (~old_lag_pm[lag_id] & (1 << p))))
          continue;
        uint32_t x =
            mc_mgr_rdm_map_get(sid, dev, pipe, mc_mgr_rdm_node_type_lag, 1);
        if (!x) goto cleanup;
        mc_mgr_rdm_addr_append(&to_write_lags, x);
      }
    }
  }

  /* Walk L1 nodes and update L2 chains with new port nodes. */
  for (s = bf_map_get_first(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node);
       BF_MAP_OK == s;
       s = bf_map_get_next(mc_mgr_ctx_db_l1(dev), &not_used, (void *)&node)) {
    if (node == NULL) {
      MC_MGR_DBGCHK(0);
      sts = BF_UNEXPECTED;
      goto cleanup;
    }

    /* Skip L1-ECMP nodes or any ECMP member nodes. They are handled separately.
     */
    if (node->ecmp_grp) continue;

    /* Skip L1 nodes which are not using the protected port as they will
     * not change. */
    bf_bitset_t ports;
    bf_dev_pipe_t pipe = mc_bit_idx_to_pipe(dev, pbit_idx);
    bf_bs_init(&ports, BF_PIPE_PORT_COUNT, node->l2_chain.ports[pipe]);

    if (!bf_bs_get(&ports, mc_bit_idx_to_local_port(dev, pbit_idx)) &&
        !mc_mgr_is_prot_port_node_lags(
            dev, node, mc_bit_idx_to_dev_port(dev, pbit_idx)))
      continue;
    /* Skip L1 nodes which are not associated as they have no HW resources
     * yet. */
    if (-1 == node->mgid) continue;
    /* Calculate the pipe mask where L1-RID nodes need to be allocated. */
    int l1_add_mask =
        mc_mgr_calculate_backup_l1_add_mask(node, old_backup, bbit_idx);
    /* Calculate the pipe mask where L1-RID nodes need to be removed. */
    int l1_del_mask =
        mc_mgr_calculate_backup_del_mask(node, old_backup, bbit_idx);
    /* Calculate pipe mask which includes old and new backup pipe mask. */
    int backup_mod_pm =
        mc_mgr_calculate_backup_mod_pm(node, old_backup, bbit_idx);

    /* Iterate through pipes and find pipes where backup port node
     * or lag nodes need to assigned. */
    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      if (!(backup_mod_pm & (1 << p))) continue;
      /* Check if an L1 needs to be added to this pipe. */
      if ((l1_add_mask & (1 << p))) {
        uint32_t l1_rdm_addr = 0;
        mc_mgr_rdm_addr_pop(&to_write, &l1_rdm_addr);
        mc_mgr_write_l1_rid_node(sid, p, node, l1_rdm_addr, 0);
      }

      /* Check if there is any existing L1-Node in this chain. If not
       * so then there is no L2 chain to be modify.  */
      if (!node->hw_nodes[p].rdm_addr) continue;
      LOG_DBG("Written L1 rdm:%#x pipe:%d rid_node:%x\n",
              node->hw_nodes[p].rdm_addr,
              p,
              node->handle);

      bf_bitset_t node_lags;
      bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);

      /* Get the address of the RDM node pointing to the port node
       * and previous node which is either a L1 or a LAG node. */
      uint32_t port_prev_addr = 0;
      mc_mgr_rdm_get_port_node_from_l1(
          node->dev, node->hw_nodes[p].rdm_addr, &addr, &port_prev_addr);
      if (addr) {
        /* If a port node already exists keep track of its address to
         * free later. */
        mc_mgr_rdm_addr_append(&to_clean, addr);
      }

      /* Check the size needed for the new port node. Write the new
       * port node. Delay the update of the prev node's next with the
       * new port node because the size of the port node might have
       * increased, so we may need to balance the tree. */
      int block_sz;
      uint8_t mask = 0;
      mc_mgr_get_port_node_size(dev, p, node, &block_sz, &mask);
      uint32_t new_l2_rdm_addr = 0;
      if (block_sz) {
        mc_mgr_rdm_addr_pop(&to_write, &new_l2_rdm_addr);
        mc_mgr_write_l2_port_node(sid, node, new_l2_rdm_addr, mask);
      }

      uint32_t l2_head_rdm = mc_mgr_rdm_l1_node_get_l2_ptr(
          mc_mgr_ctx_rdm_map(dev), node->hw_nodes[p].rdm_addr);

      /* Check if the port node is head of l2 chain, if so then assign the new
       * port node to head of the L2 chain. */
      if (port_prev_addr == node->hw_nodes[p].rdm_addr) {
        l2_head_rdm = new_l2_rdm_addr;
      }

      /* Check if we have any LAG nodes to be allocated in this pipe. */
      lag_id = -1;
      while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
        if (!bf_bs_get(&prot_port_lags, lag_id)) {
          continue;
        }

        if (!((new_lag_pm[lag_id] & (1 << p)) &&
              (~old_lag_pm[lag_id] & (1 << p)))) {
          continue;
        }

        uint32_t new_lag_rdm_addr = 0;
        mc_mgr_rdm_addr_pop(&to_write_lags, &new_lag_rdm_addr);
        mc_mgr_rdm_write_lag(sid, dev, new_lag_rdm_addr, lag_id, l2_head_rdm);
        l2_head_rdm = new_lag_rdm_addr;
      }

      /* Write the new port node and add it to the tree. */
      /* block_sz is the number of port nodes in this pipe.  Also count the
       * number of LAG nodes in this pipe so we can adjust the length stored
       * against the node struct. */
      int lag_sz = 0, port_cnt = 0;
      mc_mgr_get_l2_chain_sz(pipe, node, &port_cnt, &lag_sz);
      node->l2_count[p] = block_sz + lag_sz;

      /* Before pointing the node to the new L2 chain, check if the tail should
       * be updated.  As it is non-ECMP then only a single group can be
       * changed. */
      uint32_t l2_len = mc_mgr_compute_longest_l2(dev, p, node->mgid);
      mc_mgr_update_len(sid, dev, p, node->mgid, l2_len);

      /* port_prev_addr can point to either L1 or an existing Lag node. Thus
       * update the previous port with new port node. */
      if (port_prev_addr != node->hw_nodes[p].rdm_addr)
        mc_mgr_rdm_update_next_l2(sid, dev, port_prev_addr, new_l2_rdm_addr);

      mc_mgr_rdm_update_next_l2(
          sid, dev, node->hw_nodes[p].rdm_addr, l2_head_rdm);
    }

    /* Pushing the pending deleted nodes from the mc-mgr write list. */
    sts = mc_mgr_drv_wrl_send(sid, false);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      MC_MGR_DBGCHK(0);
    }

    /* Update the TVT/PVT accordingly. As the node RDM are already modified it
     * is not safe to return from here. Hence add an assert in error case. */
    sts = mc_mgr_update_pvt(sid, dev, node->mgid, false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, node->mgid, false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to update TVT/PVT for node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      MC_MGR_DBGCHK(0);
    }

    for (p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
      if (!(backup_mod_pm & (1 << p))) continue;
      if ((l1_del_mask & (1 << p))) {
        mc_mgr_delete_l1_node_from_pipe(sid, node, p, &to_clean);
        /* Tree may be getting shorter, check the tail. */
        uint32_t len_after_removal =
            mc_mgr_compute_longest_l2(dev, p, node->mgid);
        mc_mgr_update_len(sid, dev, p, node->mgid, len_after_removal);
        continue;
      }

      lag_id = -1;
      bf_bitset_t node_lags;
      bf_bs_init(&node_lags, BF_LAG_COUNT, node->l2_chain.lags);

      /* Check if we have LAG nodes to be removed in this pipe. These are those
       * LAG Nodes whose member port's (protected port) backup changed from
       * current pipe to some other pipe. */
      while (-1 != (lag_id = bf_bs_first_set(&node_lags, lag_id))) {
        /* Check if the LAG id is being used by the protected port */
        if (!bf_bs_get(&prot_port_lags, lag_id)) continue;

        if (!((~new_lag_pm[lag_id] & (1 << p)) &&
              (old_lag_pm[lag_id] & (1 << p))))
          continue;
        uint32_t lag_rdm_addr = 0;
        mc_mgr_rdm_unlink_and_get_lag_node_by_lag_id(
            sid, node->dev, node->hw_nodes[p].rdm_addr, &lag_rdm_addr, lag_id);
        if (!lag_rdm_addr) continue;
        mc_mgr_rdm_addr_append(&to_clean, lag_rdm_addr);
        node->l2_count[p] -= 1;
      }

      uint32_t len_after_removal =
          mc_mgr_compute_longest_l2(dev, p, node->mgid);
      mc_mgr_update_len(sid, dev, p, node->mgid, len_after_removal);
    }

    /* Update the TVT/PVT accordingly. As the node RDM are already modified it
     * is not safe to return from here. Hence add an assert in error case. */
    sts = mc_mgr_update_pvt(sid, dev, node->mgid, false, __func__, __LINE__);
    sts |= mc_mgr_update_tvt(sid, dev, node->mgid, false, __func__, __LINE__);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to update TVT/PVT for node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      MC_MGR_DBGCHK(0);
    }

    /* Mark these entries to be freed post RDM change. */
    while (to_clean) {
      uint32_t old_rdm_addr = 0;
      mc_mgr_rdm_addr_pop(&to_clean, &old_rdm_addr);
      mc_mgr_rdm_map_enqueue_free(sid, dev, old_rdm_addr);
    }

    /* Pushing the pending deleted nodes from the mc-mgr write list.
     * This also intiates RDM change for deleted nodes amd tail update
     * post DMA completion. */
    sts = mc_mgr_drv_wrl_send(sid, true);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to push mc-mgr write list node 0x%x in group 0x%x",
                node->handle,
                node->mgid);
      return sts;
    }
  }

  /* Update hardware. */
  sts = mc_mgr_set_bkup_port_wrl(sid, dev, 0, pbit_idx, bbit_idx);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(0);
    goto cleanup;
  }
  sts = mc_mgr_set_bkup_port_wrl(sid, dev, 1, pbit_idx, bbit_idx);
  if (BF_SUCCESS != sts) {
    MC_MGR_DBGCHK(0);
    goto cleanup;
  }

  MC_MGR_FREE(old_lag_pm);
  MC_MGR_FREE(new_lag_pm);
  return sts;

cleanup:
  while (to_write) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_write, &x);
    mc_mgr_rdm_map_return(dev, x);
  }
  while (to_write_lags) {
    uint32_t x = 0;
    mc_mgr_rdm_addr_pop(&to_write_lags, &x);
    mc_mgr_rdm_map_return(dev, x);
  }

  MC_MGR_FREE(old_lag_pm);
  MC_MGR_FREE(new_lag_pm);
  return sts;
}

bf_status_t mc_mgr_get_c2c(int sid,
                           bf_dev_id_t dev,
                           bf_dev_port_t *port,
                           bool *enable) {
  (void)sid;
  *enable = mc_mgr_ctx_cpu_port_en(dev);
  *port = mc_mgr_ctx_cpu_port(dev);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_set_c2c(int sid,
                           bf_dev_id_t dev,
                           bool enable,
                           bf_dev_port_t port) {
  bool old_en = mc_mgr_ctx_cpu_port_en(dev);
  bf_dev_port_t old_port = mc_mgr_ctx_cpu_port(dev);

  mc_mgr_ctx_cpu_port_en_set(dev, enable);
  mc_mgr_ctx_cpu_port_set(dev, port);

  bf_status_t sts = mc_mgr_set_pre_ctrl_wrl(sid, dev);
  if (BF_SUCCESS != sts) {
    mc_mgr_ctx_cpu_port_en_set(dev, old_en);
    mc_mgr_ctx_cpu_port_set(dev, old_port);
    mc_mgr_drv_wrl_abort(sid);
  }

  return sts;
}

bf_status_t mc_mgr_set_l1_time_slice(int sid, bf_dev_id_t dev, int count) {
  int old_count = mc_mgr_ctx_max_l1_ts(dev);
  mc_mgr_ctx_max_l1_ts_set(dev, count);

  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_set_pre_ctrl_wrl(sid, dev);
  if (sts != BF_SUCCESS) {
    mc_mgr_ctx_max_l1_ts_set(dev, old_count);
    mc_mgr_drv_wrl_abort(sid);
  }
  return sts;
}

bf_status_t mc_mgr_set_max_nodes(int sid, bf_dev_id_t dev, int l1, int l2) {
  /* Increase the l1 count by 1 to account for the tail. */
  if (l1 + 1 < 0xFFFFF) l1 += 1;
  /* Increase the l2 count by 260 to account for the tail. */
  if (l2 + 260 < 0xFFFFF)
    l2 += 260;
  else
    l2 = 0xFFFFF;

  int old_l1 = mc_mgr_ctx_max_l1(dev);
  int old_l2 = mc_mgr_ctx_max_l2(dev);
  mc_mgr_ctx_max_l1_set(dev, l1);
  mc_mgr_ctx_max_l2_set(dev, l2);

  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_set_max_l1_wrl(sid, dev);
  if (BF_SUCCESS == sts) {
    sts = mc_mgr_set_max_l2_wrl(sid, dev);
  }
  if (BF_SUCCESS != sts) {
    mc_mgr_ctx_max_l1_set(dev, old_l1);
    mc_mgr_ctx_max_l2_set(dev, old_l2);
    mc_mgr_drv_wrl_abort(sid);
  }
  return sts;
}

bf_status_t mc_mgr_get_lag_hw(bf_dev_id_t dev,
                              int ver,
                              int id,
                              bf_bitset_t *lag) {
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    return BF_INVALID_ARG;
  }
  if (0 > id || BF_LAG_COUNT <= id) {
    return BF_INVALID_ARG;
  }
  bf_status_t sts = BF_SUCCESS;
  for (int p = 0; p < (int)mc_mgr_ctx_num_max_pipes(dev); ++p) {
    sts = mc_mgr_get_lit_seg_reg(dev, ver, id, p, &lag[p]);
    if (BF_SUCCESS != sts) return sts;
  }
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_lag_np_hw(
    bf_dev_id_t dev, int ver, int id, int *l_cnt, int *r_cnt) {
  if (!mc_mgr_validate_dev(dev, __func__, __LINE__)) {
    return BF_INVALID_ARG;
  }
  if (0 != ver && 1 != ver) {
    return BF_INVALID_ARG;
  }
  if (0 > id || BF_LAG_COUNT <= id) {
    return BF_INVALID_ARG;
  }
  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_get_lit_np_reg(dev, ver, id, l_cnt, r_cnt);
  return sts;
}

bf_status_t mc_get_port_prune_table_size(bf_dev_id_t dev_id, uint32_t *count) {
  bf_status_t sts = BF_SUCCESS;
  switch (mc_mgr_ctx_dev_family(dev_id)) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      *count = BF_PIPE_PORT_COUNT * 4;
      break;
    default:
      sts = BF_INVALID_ARG;
      break;
  }

  return sts;
}

bf_status_t mc_mgr_update_yid_tbl(int sid,
                                  bf_dev_id_t dev,
                                  int yid,
                                  bf_mc_port_map_t port_map) {
  bf_status_t sts = BF_SUCCESS;

  /* Copy the new prune mask to the SW shadow. */
  size_t i;
  bf_bitset_t *pmt = mc_mgr_ctx_pmt(dev, yid);
  for (i = 0; i < sizeof(bf_mc_port_map_t); ++i) {
    bf_bs_set_word(pmt, 8 * i, 8, port_map[i]);
  }

  /* Program the HW from the shadow. */
  sts = mc_mgr_set_pmt_wrl(sid, dev, 0, yid);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program PMT0, dev %d id %d", dev, yid);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }
  sts = mc_mgr_set_pmt_wrl(sid, dev, 1, yid);
  if (sts != BF_SUCCESS) {
    LOG_ERROR("Failed to program PMT1, dev %d id %d", dev, yid);
    mc_mgr_drv_wrl_abort(sid);
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    return sts;
  }

  return sts;
}

bf_status_t mc_mgr_set_global_rid(bf_dev_id_t dev, uint16_t rid) {
  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_set_global_rid_reg(dev, rid);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to write global rid on dev %d to %#x, sts %s (%d)",
              dev,
              rid,
              bf_err_str(sts),
              sts);
  } else {
    mc_mgr_ctx_glb_rid_set(dev, rid);
  }
  return sts;
}

bf_status_t mc_mgr_get_global_rid(bf_dev_id_t dev, uint16_t *rid) {
  *rid = mc_mgr_ctx_glb_rid(dev);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_get_blk_id_grp_hw(bf_dev_id_t dev, int idx, uint32_t *id) {
  bf_status_t sts = BF_SUCCESS;
  sts = mc_mgr_get_rdm_blk_id_grp_reg(dev, idx, id);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to read RDM block id on dev %d, sts %s (%d)",
              dev,
              bf_err_str(sts),
              sts);
  }
  return sts;
}

/* mc_mgr_enable_all_dr
 *
 * Enables all the DRs used by the mc manager
 */
void mc_mgr_enable_all_dr(bf_dev_id_t dev_id) {
  bf_dma_dr_id_t dr_id_tx, dr_id_cmp;
  switch (mc_mgr_ctx_dev_family(dev_id)) {
    case BF_DEV_FAMILY_TOFINO:
      dr_id_tx = lld_dr_tx_que_write_list;
      dr_id_cmp = lld_dr_cmp_que_write_list;
      mcmgr_tm_set_dr_state(dev_id, true);
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      dr_id_tx = lld_dr_tx_que_write_list_1;
      dr_id_cmp = lld_dr_cmp_que_write_list_1;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return;
  }

  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev_id);
       subdev++) {
    // Enable the completion DR
    lld_subdev_dr_enable_set(dev_id, subdev, dr_id_cmp, true);

    // Enable the Tx DR
    lld_subdev_dr_enable_set(dev_id, subdev, dr_id_tx, true);
  }
}

/* mc_mgr_disable_all_dr
 *
 * Disables all the DRs used by the mc manager
 */
void mc_mgr_disable_all_dr(bf_dev_id_t dev_id) {
  bf_dma_dr_id_t dr_id_tx, dr_id_cmp;
  switch (mc_mgr_ctx_dev_family(dev_id)) {
    case BF_DEV_FAMILY_TOFINO:
      dr_id_tx = lld_dr_tx_que_write_list;
      dr_id_cmp = lld_dr_cmp_que_write_list;
      mcmgr_tm_set_dr_state(dev_id, false);
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      dr_id_tx = lld_dr_tx_que_write_list_1;
      dr_id_cmp = lld_dr_cmp_que_write_list_1;
      break;
    default:
      MC_MGR_DBGCHK(0);
      return;
  }

  for (uint32_t subdev = 0; subdev < mc_mgr_ctx_num_subdevices(dev_id);
       subdev++) {
    // Disable the Tx DR
    lld_subdev_dr_enable_set(dev_id, subdev, dr_id_tx, false);

    // Disable the completion DR
    lld_subdev_dr_enable_set(dev_id, subdev, dr_id_cmp, false);
  }
}

static bf_status_t mc_mgr_lock_device(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  LOG_TRACE("Entering %s, dev %d ", __func__, dev_id);

  if (dev_id >= MC_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_mgr_ctx_dev_locked_set(dev_id, true);

  mc_mgr_one_at_a_time_end();

  return status;
}

static bf_status_t mc_mgr_create_dma(bf_dev_id_t dev_id) {
  bf_status_t sts;
  bf_mc_session_hdl_t shdl = mc_mgr_ctx_int_sess();
  int sid = mc_mgr_validate_session(shdl, __func__, __LINE__);
  int i, v, d = dev_id;

  mc_mgr_ctx_rebuilding_set(dev_id);

  /* Write max_l1_slice. */
  sts = bf_mc_set_max_nodes_before_yield(shdl, dev_id, mc_mgr_ctx_max_l1_ts(d));
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "max-nodes-before-yield",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }

  /* Write CPU port config. */
  sts =
      mc_mgr_set_c2c(sid, d, mc_mgr_ctx_cpu_port_en(d), mc_mgr_ctx_cpu_port(d));
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "copy-to-cpu",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }
  /* Write max L1 and L2 nodes. */
  sts = bf_mc_set_max_node_threshold(
      shdl, dev_id, mc_mgr_ctx_max_l1(d), mc_mgr_ctx_max_l2(d));
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "max-node-threshold",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }
  /* Write the Fast Failover enable. */
  if (mc_mgr_ctx_ff_en(d))
    sts = bf_mc_enable_port_fast_failover(shdl, dev_id);
  else
    sts = bf_mc_disable_port_fast_failover(shdl, dev_id);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "fast-failover",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }
  /* Write the Backup Port enable. */
  if (mc_mgr_ctx_bkup_port_en(d))
    sts = bf_mc_enable_port_protection(shdl, dev_id);
  else
    sts = bf_mc_disable_port_protection(shdl, dev_id);
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "port-protection",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }
  /* Write the global RID. */
  sts = bf_mc_set_global_exclusion_rid(shdl, dev_id, mc_mgr_ctx_glb_rid(d));
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "global-rid",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }
  /* Write the SW port forwarding state. */
  for (v = 0; v < 2; ++v) {
    for (i = 0;
         i < (int)MC_MGR_CALC_PORT_FWD_STATE_COUNT(mc_mgr_ctx_num_max_ports(d));
         i += 32) {
      sts = mc_mgr_set_port_mask_wrl(sid, d, v, i);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                  "port-mask",
                  dev_id,
                  bf_err_str(sts));
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        goto done;
      }
    }
  }
  /* Write the remote LAG counts. */
  for (i = 0; i < BF_LAG_COUNT; ++i) {
    sts = mc_mgr_lag_update_rmt_cnt(
        sid, dev_id, i, mc_mgr_ctx_lit_np_l(d, i), mc_mgr_ctx_lit_np_r(d, i));
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                "remote-lag-count",
                dev_id,
                bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }
  /* Write the LIT. */
  for (v = 0; v < 2; ++v) {
    for (i = 0; i < BF_LAG_COUNT; ++i) {
      sts = mc_mgr_set_lit_wrl(sid, d, v, i);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                  "lag membership",
                  dev_id,
                  bf_err_str(sts));
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        goto done;
      }
    }
  }

  /* Write the PMT. */
  for (v = 0; v < 2; ++v) {
    for (i = 0;
         i < (int)(BF_SUBDEV_PORT_COUNT * mc_mgr_ctx_num_subdevices(dev_id));
         ++i) {
      sts = mc_mgr_set_pmt_wrl(sid, d, v, i);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                  "port-prune-table",
                  dev_id,
                  bf_err_str(sts));
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        goto done;
      }
    }
  }
  /* Write the Backup Port Table. */
  for (v = 0; v < 2; ++v) {
    for (i = 0; i < (int)mc_mgr_ctx_num_max_ports(dev_id); ++i) {
      sts = mc_mgr_set_bkup_port_wrl(sid, d, v, i, mc_mgr_ctx_bkup_port(d, i));
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                  "backup-port-table",
                  dev_id,
                  bf_err_str(sts));
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        goto done;
      }
    }
  }
  /* Write the table version. */
  sts = mc_mgr_program_tbl_ver(sid, d, mc_mgr_ctx_tbl_ver(d));
  if (BF_SUCCESS != sts) {
    LOG_ERROR("Failed to setup %s on dev %d, sts %s",
              "multicast-version",
              dev_id,
              bf_err_str(sts));
    MC_MGR_DBGCHK(BF_SUCCESS == sts);
    goto done;
  }
  /* Write the MIT. */
  int pipe, mgid;
  for (pipe = 0; pipe < (int)mc_mgr_ctx_num_max_pipes(dev_id); ++pipe) {
    for (mgid = 0; mgid < BF_MGID_COUNT; mgid += 4) {
      sts = mc_mgr_set_mit_wrl(sid, d, pipe, mgid);
      if (BF_SUCCESS != sts) {
        LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                  "multicast-index-table",
                  dev_id,
                  bf_err_str(sts));
        MC_MGR_DBGCHK(BF_SUCCESS == sts);
        goto done;
      }
    }
  }
  /* Write the PVT. */
  for (uint32_t r = 0; r < mc_mgr_ctx_pvt_sz(d); ++r) {
    sts = mc_mgr_pvt_write_row_from_shadow(
        d, sid, BF_DEV_PIPE_ALL, r, MC_PVT_MASK_ALL, true);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                "pipe-vector-table",
                dev_id,
                bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }
  /* Write the TVT. */
  for (uint32_t r = 0; r < mc_mgr_ctx_tvt_sz(d); ++r) {
    sts = mc_mgr_tvt_write_row_from_shadow(d, BF_DEV_PIPE_ALL, r, true);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                "TVT-table",
                dev_id,
                bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }
  /* Write the RDM block assignments. */
  int rdm_blk_count, rdm_line_count;
  if (mc_mgr_ctx_p == NULL) {
    sts = BF_INVALID_ARG;
    goto done;
  }
  int rdm_range = 16;
  switch (mc_mgr_ctx_p->dev_ctx[dev_id]->dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      rdm_blk_count = TOF_MC_MGR_RDM_BLK_COUNT;
      rdm_line_count = TOF_MC_MGR_RDM_LINE_COUNT;
      rdm_range = 16;
      break;
    case BF_DEV_FAMILY_TOFINO2:
      rdm_blk_count = TOF2_MC_MGR_RDM_BLK_COUNT;
      rdm_line_count = TOF2_MC_MGR_RDM_LINE_COUNT;
      rdm_range = 16;
      break;
    case BF_DEV_FAMILY_TOFINO3:
      rdm_blk_count = TOF3_MC_MGR_RDM_BLK_COUNT;
      rdm_line_count = TOF3_MC_MGR_RDM_LINE_COUNT;
      rdm_range = 8;
      break;
    default:
      MC_MGR_DBGCHK(0);
      sts = BF_INVALID_ARG;
      goto done;
  }
  for (i = 0; i < (rdm_blk_count + (rdm_range - 1)) / rdm_range; ++i) {
    sts = mc_mgr_set_rdm_blk_id_grp_wrl(sid, d, i);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                "RDM-block-id",
                dev_id,
                bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }
  /* Write the RDM. */
  for (i = 0; i < rdm_line_count; ++i) {
    if (rdm_line_invalid(&mc_mgr_ctx_rdm_map(dev_id)->rdm[i])) continue;
    mc_mgr_rdm_line_t *line = &mc_mgr_ctx_rdm_map(dev_id)->rdm[i];
    sts = mc_mgr_set_rdm_wrl(sid, d, i, line->data[1], line->data[0]);
    if (BF_SUCCESS != sts) {
      LOG_ERROR("Failed to setup %s on dev %d, sts %s",
                "RDM-Contents",
                dev_id,
                bf_err_str(sts));
      MC_MGR_DBGCHK(BF_SUCCESS == sts);
      goto done;
    }
  }

done:
  mc_mgr_ctx_rebuilding_clr(dev_id);
  return sts;
}

static bf_status_t mc_mgr_unlock_device(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  int i = 0;

  LOG_TRACE("Entering %s, dev %d", __func__, dev_id);

  if (dev_id >= MC_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }

  // Enable all the DRs
  mc_mgr_enable_all_dr(dev_id);

  mc_mgr_one_at_a_time_begin();

  mc_mgr_ctx_dev_locked_set(dev_id, false);

  /* Push the write lists */
  for (i = 0; i < MC_MGR_NUM_SESSIONS; ++i) {
    if (mc_mgr_ctx_session_state(i)->valid) (void)mc_mgr_drv_wrl_send(i, true);
  }
  mc_mgr_one_at_a_time_end();

  return status;
}
static bf_status_t mc_mgr_config_complete(bf_dev_id_t dev_id) {
  mc_mgr_drv_cmplt_operations(0, dev_id);
  return BF_SUCCESS;
}

bf_status_t mc_mgr_hitless_ha_hw_read(bf_dev_id_t dev_id) {
  if (MC_MGR_INVALID_DEV(dev_id)) {
    LOG_ERROR("Invalid device %u at %s", dev_id, __func__);
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }

  struct mc_mgr_dev_hw_state **state = mc_mgr_ctx_ha_state(dev_id);
  if (*state) {
    LOG_ERROR("HW state already present (%p), ignoring it", (void *)*state);
    *state = NULL;
  }

  *state = MC_MGR_MALLOC(sizeof(struct mc_mgr_dev_hw_state));
  if (!*state) {
    LOG_ERROR("Could not allocate memory to hold HW state");
    MC_MGR_DBGCHK(0);
    return BF_NO_SYS_RESOURCES;
  }
  MC_MGR_MEMSET(*state, 0, sizeof(struct mc_mgr_dev_hw_state));

  int sid;
  if (!mc_mgr_decode_sess_hdl(mc_mgr_ctx_int_sess(), &sid)) {
    LOG_ERROR("Invalid internal session handle, %#x", mc_mgr_ctx_int_sess());
    MC_MGR_DBGCHK(0);
    return BF_INVALID_ARG;
  }
  bf_status_t sts = mc_mgr_read_hw_state(sid, dev_id, *state);
  if (BF_SUCCESS != sts) {
    LOG_ERROR(
        "Failed to read state from device %d, %s", dev_id, bf_err_str(sts));
  }
  return sts;
}

bf_status_t mc_mgr_push_delta_changes(bf_dev_id_t dev_id) {
  bf_status_t status = BF_SUCCESS;
  int i = 0;

  if (dev_id >= MC_MGR_NUM_DEVICES) {
    return BF_INVALID_ARG;
  }
  if (!mc_mgr_ctx()->dev_ctx[dev_id]) {
    /* We don't know this device, hence nothing to do */
    return BF_SUCCESS;
  }
  mc_mgr_one_at_a_time_begin();

  /* Push the write lists */
  mc_mgr_ctx_dev_locked_set(dev_id, false);
  mc_mgr_ctx_rebuilding_clr(dev_id);
  for (i = 0; i < MC_MGR_NUM_SESSIONS; ++i) {
    if (mc_mgr_ctx_session_state(i)->valid) mc_mgr_drv_wrl_send(i, true);
  }
  mc_mgr_one_at_a_time_end();

  return status;
}

bool mc_mgr_is_device_locked(bf_dev_id_t dev_id) {
  if (MC_MGR_INVALID_DEV(dev_id)) {
    MC_MGR_DBGCHK(0);
    return false;
  }
  return mc_mgr_ctx_dev_locked(dev_id);
}

bool mc_mgr_versioning_on(int sid, bf_dev_id_t dev_id) {
  if (MC_MGR_INVALID_SID(sid)) {
    MC_MGR_DBGCHK(0);
    return false;
  }
  bool locked = mc_mgr_is_device_locked(dev_id);
  bool batch = mc_mgr_in_batch(sid);
  return (!batch && !locked);
}

bool mc_mgr_in_batch(int sid) {
  if (MC_MGR_INVALID_SID(sid)) {
    return false;
  }
  return (mc_mgr_ctx_in_batch(sid));
}

void mc_mgr_begin_batch(int sid) {
  if (MC_MGR_INVALID_SID(sid)) {
    return;
  }

  if (mc_mgr_in_batch(sid)) return;

  /* Take the lock now and don't give it up until the batch end. */
  mc_mgr_one_at_a_time_begin();
  mc_mgr_ctx_in_batch_set(sid, true);
}

bf_status_t mc_mgr_flush_batch(int sid) {
  if (MC_MGR_INVALID_SID(sid)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_mgr_ctx_in_batch_set(sid, false);
  bf_status_t sts = mc_mgr_drv_wrl_send(sid, true);
  mc_mgr_ctx_in_batch_set(sid, true);

  mc_mgr_one_at_a_time_end();
  return sts;
}

bf_status_t mc_mgr_end_batch(int sid) {
  if (MC_MGR_INVALID_SID(sid)) {
    return BF_INVALID_ARG;
  }

  mc_mgr_one_at_a_time_begin();

  mc_mgr_ctx_in_batch_set(sid, false);
  bf_status_t sts = mc_mgr_drv_wrl_send(sid, true);

  /* One extra unlock here to account for the extra lock taken when the batch
   * started. */
  mc_mgr_one_at_a_time_end();
  mc_mgr_one_at_a_time_end();
  return sts;
}
