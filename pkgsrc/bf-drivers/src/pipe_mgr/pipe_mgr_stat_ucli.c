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


/*!
 * @file pipe_mgr_stat_ucli.c
 * @date
 *
 * Stat table manager ucli commands, command-handlers and the world.
 */

/* Standard header includes */
#include <getopt.h>
#include <limits.h>

/* Module header includes */

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_stat_mgr_dump.h"
#include "pipe_mgr_stat_mgr_int.h"
#include "pipe_mgr_stat_trace.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_stat_tbl_ucli_ucli__##name##__
#define PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(name)                               \
  static ucli_status_t PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * \
                                                             uc)

extern bool stat_mgr_enable_detail_trace;

PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(tbl_info) {
  PIPE_MGR_CLI_PROLOGUE(
      "tbl_info", "Dump stat table info", "tbl_info -d <dev_id> -h <tbl_hdl>");
  bf_dev_id_t device_id = 0;
  pipe_stat_tbl_hdl_t stat_tbl_hdl = 0;

  int c;
  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        device_id = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        stat_tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (device_id < 0 || device_id >= PIPE_MGR_NUM_DEVICES || 0 == stat_tbl_hdl) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_stat_dump_tbl_info(uc, device_id, stat_tbl_hdl);
  return UCLI_STATUS_OK;
}

PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(entry_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, pflag, eflag;
  char *d_arg, *h_arg, *p_arg, *e_arg;
  int argc;
  uint8_t device_id;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  pipe_stat_ent_idx_t entry_idx;
  bf_dev_pipe_t pipe_id;
  char *const *argv;
  static char usage[] =
      "Usage : entry_info -d <dev_id> -h <tbl_hdl> -p <pipe_id> -e "
      "<entry_idx>/\n";

  UCLI_COMMAND_INFO(uc,
                    "entry_info",
                    -1,
                    "Dump stat table entry info"
                    " Usage : entry_info -d <dev_id> -h <tbl_hdl> -p <pipe_id> "
                    "-e <entry_idx>\n");

  dflag = hflag = pflag = eflag = 0;
  d_arg = h_arg = p_arg = e_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:p:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else /*if (dflag == 1)*/
  {
    device_id = strtoul(d_arg, NULL, 8);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "tbl_info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    stat_tbl_hdl = strtoul(h_arg, NULL, 16);
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 10);
  } else {
    /* Pipe id flag was not passed, just assume BF_DEV_PIPE_ALL */
    pipe_id = BF_DEV_PIPE_ALL;
  }

  if (eflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    entry_idx = strtoul(e_arg, NULL, 10);
  }

  pipe_mgr_stat_dump_entry_info(
      uc, device_id, pipe_id, stat_tbl_hdl, entry_idx);

  return UCLI_STATUS_OK;
}

static pipe_mgr_stat_tbl_t *get_stat_tbl(ucli_context_t *uc,
                                         bf_dev_id_t dev_id,
                                         pipe_mat_tbl_hdl_t tbl_hdl) {
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  pipe_stat_tbl_hdl_t stat_tbl_hdl = 0;

  switch (PIPE_GET_HDL_TYPE(tbl_hdl)) {
    case PIPE_HDL_TYPE_MAT_TBL: {
      pipe_mat_tbl_info_t *mat =
          pipe_mgr_get_tbl_info(dev_id, tbl_hdl, __func__, __LINE__);
      if (!mat) {
        aim_printf(
            &uc->pvs, "Cannot find table 0x%x on device %d\n", tbl_hdl, dev_id);
        return NULL;
      }
      if (mat->stat_tbl_ref) {
        stat_tbl_hdl = mat->stat_tbl_ref->tbl_hdl;
        break;
      }
      aim_printf(&uc->pvs,
                 "Table 0x%x on device %d has no stat table references\n",
                 tbl_hdl,
                 dev_id);
      return NULL;
    }
    case PIPE_HDL_TYPE_STAT_TBL:
      stat_tbl_hdl = tbl_hdl;
      break;
    default:
      aim_printf(&uc->pvs,
                 "Table 0x%x on device %d is neither a MAT nor stat table\n",
                 tbl_hdl,
                 dev_id);
      return NULL;
  }
  stat_tbl = pipe_mgr_stat_tbl_get(dev_id, stat_tbl_hdl);
  if (!stat_tbl) {
    aim_printf(&uc->pvs,
               "Cannot find stat table 0x%x on device %d\n",
               stat_tbl_hdl,
               dev_id);
  }
  return stat_tbl;
}

PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(handle_info) {
  PIPE_MGR_CLI_PROLOGUE("handle_info",
                        " Dump entry handle info for direct stat tbl",
                        "-d <dev> -h <table handle> -e <MAT entry handle>");
  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  pipe_mat_ent_hdl_t entry_hdl = 0;

  int c;
  while ((c = getopt(argc, argv, "d:h:e:")) != -1) {
    switch (c) {
      case 'd':
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      case 'e':
        entry_hdl = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dev_id < 0 || dev_id >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev_id);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_stat_tbl_t *stat_tbl = get_stat_tbl(uc, dev_id, tbl_hdl);
  if (!stat_tbl) return UCLI_STATUS_OK;

  if (stat_tbl->ref_type != PIPE_TBL_REF_TYPE_DIRECT) {
    aim_printf(&uc->pvs,
               "Table %s (0x%x) on device %d is indirect\n",
               stat_tbl->name,
               stat_tbl->stat_tbl_hdl,
               dev_id);
    return UCLI_STATUS_OK;
  }
  pipe_mgr_stat_tbl_instance_t *stat_tbl_instance;
  if (stat_tbl->symmetric) {
    stat_tbl_instance = stat_tbl->stat_tbl_instances;
    if (!stat_tbl_instance) {
      aim_printf(&uc->pvs,
                 "Cannot find stat table 0x%x instance for pipe ALL\n",
                 stat_tbl->stat_tbl_hdl);
      return UCLI_STATUS_OK;
    }
  } else {
    bf_dev_pipe_t hdl_pipe = PIPE_GET_HDL_PIPE(entry_hdl);
    stat_tbl_instance = pipe_mgr_stat_tbl_get_instance(stat_tbl, hdl_pipe);
    if (!stat_tbl_instance) {
      aim_printf(
          &uc->pvs,
          "Cannot find stat table 0x%x instance for pipe %d, from entry %u\n",
          stat_tbl->stat_tbl_hdl,
          hdl_pipe,
          entry_hdl);
      return UCLI_STATUS_OK;
    }
  }

  aim_printf(&uc->pvs,
             "Dev %d tbl 0x%x (%s) inst %x entry %u (0x%x)\n",
             dev_id,
             stat_tbl->stat_tbl_hdl,
             stat_tbl->name,
             stat_tbl_instance->pipe_id,
             entry_hdl,
             entry_hdl);

  aim_printf(&uc->pvs, "  Locations:\n");
  PIPE_MGR_LOCK(&stat_tbl_instance->ent_hdl_loc_mtx);
  unsigned long key = entry_hdl;
  pipe_mgr_stat_mgr_ent_hdl_loc_t *ent_hdl_loc = NULL;
  bf_map_sts_t msts =
      bf_map_get(&stat_tbl_instance->ent_hdl_loc, key, (void **)&ent_hdl_loc);
  if (msts == BF_MAP_OK && ent_hdl_loc->locations) {
    aim_printf(&uc->pvs,
               "    "
               "LogPipe|CurStage|CurIndex|DstStage|DstIndex|SetCount|Pending|"
               "Del\n");
    aim_printf(&uc->pvs,
               "    "
               "-------+--------+--------+--------+--------+--------|-------+"
               "---\n");
    for (unsigned int pipe = 0;
         pipe < stat_tbl->dev_info->dev_cfg.num_pipelines;
         ++pipe) {
      for (pipe_mgr_stat_ent_location_t *l = ent_hdl_loc->locations; l;
           l = l->next) {
        if (l->pipe_id != pipe) continue;
        aim_printf(&uc->pvs,
                   "    %7d|%8d|%8d|%8d|%8d|%8d|%7s|%3s\n",
                   l->pipe_id,
                   l->cur_stage_id,
                   l->cur_ent_idx,
                   l->def_stage_id,
                   l->def_ent_idx,
                   l->def_set_in_prog,
                   l->pending ? "T" : "F",
                   l->entry_del_in_progress ? "T" : "F");
      }
    }
  } else {
    aim_printf(&uc->pvs, "  None:\n");
  }
  PIPE_MGR_UNLOCK(&stat_tbl_instance->ent_hdl_loc_mtx);
  return UCLI_STATUS_OK;
}

/* If no entry specified, load entire table with requested value except for the
 * last entry which is loaded with all 1s.
 * If no table handle specified, apply to all tables. */
PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(load_cntr) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag, pflag, eflag, vflag;
  char *d_arg, *h_arg, *p_arg, *e_arg, *v_arg;
  int argc;
  uint8_t device_id = 0;
  pipe_stat_tbl_hdl_t stat_tbl_hdl = 0;
  pipe_stat_ent_idx_t entry_idx = 0;
  bf_dev_pipe_t pipe_id;
  uint64_t value;
  char *const *argv;
  static char usage[] =
      "usage : load-cntr -d <dev_id> [-h <tbl_hdl>] [-p <pipe_id>] [-e "
      "<entry_idx>] -v <value>\n";

  UCLI_COMMAND_INFO(uc,
                    "load-cntr",
                    -1,
                    "Load HW counter values"
                    " load-cntr -d <dev_id> [-h <tbl_hdl>] [-p <pipe_id>] [-e "
                    "<entry_idx>] -v <value>\n");

  dflag = hflag = pflag = eflag = vflag = 0;
  d_arg = h_arg = p_arg = e_arg = v_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:p:e:v:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'v':
        vflag = 1;
        v_arg = optarg;
        if (!v_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else /*if (dflag == 1)*/
  {
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(&uc->pvs, "load-cntr : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }

  if (hflag != 0) {
    stat_tbl_hdl = strtoul(h_arg, NULL, 0);
  }

  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
    if (pipe_id != BF_DEV_PIPE_ALL && pipe_id >= BF_PIPE_COUNT) {
      aim_printf(&uc->pvs, "Pipe id should be less then %d\n", BF_PIPE_COUNT);
      return UCLI_STATUS_OK;
    }
  } else {
    /* Pipe id flag was not passed, just assume BF_DEV_PIPE_ALL */
    pipe_id = BF_DEV_PIPE_ALL;
  }

  if (eflag != 0) {
    entry_idx = strtoul(e_arg, NULL, 0);
  }

  if (vflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    value = strtoull(v_arg, NULL, 0);
  }

  pipe_stat_tbl_hdl_t h = 0;
  if (hflag)
    h = stat_tbl_hdl;
  else
    h = PIPE_SET_HDL_TYPE(h, PIPE_HDL_TYPE_STAT_TBL);
  for (; (h & 0xFFF) < 1000; ++h) {
    pipe_stat_data_t data = {.bytes = value, .packets = value};
    pipe_mgr_stat_tbl_t *stat_tbl = pipe_mgr_stat_tbl_get(device_id, h);
    if (stat_tbl == NULL && hflag) {
      aim_printf(&uc->pvs,
                 "No information found for the stat table specified %#x\n",
                 h);
      return UCLI_STATUS_OK;
    } else if (stat_tbl == NULL) {
      continue;
    } else if (stat_tbl->over_allocated) {
      dev_target_t d = {.device_id = device_id, .dev_pipe_id = pipe_id};
      if (eflag) {
        pipe_status_t x = pipe_mgr_stat_ent_load(
            0, d, h, (pipe_stat_ent_idx_t)entry_idx, &data);
        if (PIPE_SUCCESS != x)
          aim_printf(&uc->pvs,
                     "Error %d (%s) loading stat entry\n",
                     x,
                     pipe_str_err(x));
      } else {
        uint32_t i;
        for (i = 0; i < stat_tbl->stat_tbl_instances[0].capacity - 1; ++i) {
          pipe_status_t x =
              pipe_mgr_stat_ent_load(0, d, h, (pipe_stat_ent_idx_t)i, &data);
          if (PIPE_SUCCESS != x)
            aim_printf(&uc->pvs,
                       "Error %d (%s) loading stat entry\n",
                       x,
                       pipe_str_err(x));
        }
        data.packets = UINT64_C(0xFFFFFFFFFFFFFFFF);
        data.bytes = UINT64_C(0xFFFFFFFFFFFFFFFF);
        pipe_status_t x =
            pipe_mgr_stat_ent_load(0, d, h, (pipe_stat_ent_idx_t)i, &data);
        if (PIPE_SUCCESS != x)
          aim_printf(&uc->pvs,
                     "Error %d (%s) loading stat entry\n",
                     x,
                     pipe_str_err(x));
      }
    } else {
      if (!hflag) continue;
      pipe_status_t x =
          pipe_mgr_mat_ent_direct_stat_load(0, device_id, h, entry_idx, &data);
      if (PIPE_SUCCESS != x)
        aim_printf(
            &uc->pvs, "Error %d (%s) loading stat entry\n", x, pipe_str_err(x));
    }

    if (hflag) break;
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(lrt_evict_entry_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, pflag, hflag, eflag;
  char *d_arg, *p_arg, *h_arg, *e_arg;
  int argc;
  uint8_t device_id = 0;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL;
  uint8_t stage_id = 0;
  unsigned i;
  char *const *argv;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  pipe_mgr_stat_tbl_t *stat_tbl = NULL;
  pipe_mgr_stat_tbl_instance_t *stat_tbl_instance = NULL;
  pipe_stat_ent_idx_t entry_idx;
  pipe_stat_stage_ent_idx_t stage_ent_idx;
  static char usage[] =
      "usage :lrt evict entry info for a counter index -d <dev_id> -h "
      "<tbl_hdl> [-p "
      "<pipe_id>]\n -e <entry_idx>\n";
  UCLI_COMMAND_INFO(uc,
                    "lrt-evict-info",
                    -1,
                    "Show lrt evict info on the entry index"
                    " lrt-evict-info -d <dev_id> -h <tbl_hdl> -p <pipe_id> "
                    "-e <entry_idx>\n");
  dflag = pflag = hflag = eflag = 0;
  d_arg = p_arg = h_arg = e_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:p:h:e:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'p':
        pflag = 1;
        p_arg = optarg;
        if (!p_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'e':
        eflag = 1;
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else { /*if (dflag == 1)*/
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(
          &uc->pvs, "lrt-evict-info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }
  if (pflag == 1) {
    pipe_id = strtoul(p_arg, NULL, 0);
    if (pipe_id >= BF_PIPE_COUNT) {
      aim_printf(&uc->pvs, "Pipe id should be less then %d\n", BF_PIPE_COUNT);
      return UCLI_STATUS_OK;
    }
  } else {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }

  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    stat_tbl_hdl = strtoul(h_arg, NULL, 16);
  }

  if (eflag == 0 || e_arg == NULL) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    entry_idx = strtoul(e_arg, NULL, 10);
  }
  stat_tbl = pipe_mgr_stat_tbl_get(device_id, stat_tbl_hdl);
  if (stat_tbl == NULL) {
    aim_printf(&uc->pvs, "Stat table does not exist\n");
    return UCLI_STATUS_OK;
  }
  if (stat_tbl->lrt_enabled == false) {
    aim_printf(&uc->pvs, "LRT not enabled for table 0x%x\n", stat_tbl_hdl);
    return UCLI_STATUS_OK;
  }
  if (stat_tbl->symmetric) {
    stat_tbl_instance = &stat_tbl->stat_tbl_instances[0];
  } else {
    for (i = 0; i < stat_tbl->num_instances; i++) {
      if (pipe_id == stat_tbl->stat_tbl_instances[i].pipe_id) {
        stat_tbl_instance = &stat_tbl->stat_tbl_instances[i];
        break;
      }
    }
  }
  if (!stat_tbl_instance) {
    aim_printf(&uc->pvs, "Stat table instance does not exist\n");
    return UCLI_STATUS_OK;
  }
  stage_id = pipe_mgr_stat_mgr_ent_get_stage(
      stat_tbl, stat_tbl_instance, entry_idx, 0xff, &stage_ent_idx);
  aim_printf(
      &uc->pvs,
      "Number of LRT evicts rcvd : %d\n",
      stat_tbl_instance->ent_idx_lrt_dbg_info[pipe_id][stage_id][stage_ent_idx]
          .num_lrt_evicts_rcvd);
  aim_printf(
      &uc->pvs,
      "Packet counter in last update : %" PRIu64 "\n",
      stat_tbl_instance->ent_idx_lrt_dbg_info[pipe_id][stage_id][stage_ent_idx]
          .stat_data.packets);
  aim_printf(
      &uc->pvs,
      "Byte counter in last update : %" PRIu64 "\n",
      stat_tbl_instance->ent_idx_lrt_dbg_info[pipe_id][stage_id][stage_ent_idx]
          .stat_data.bytes);
  return UCLI_STATUS_OK;
}

PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(lrt_evict_tbl_info) {
  extern char *optarg;
  extern int optind;
  int c, dflag, hflag;
  char *d_arg, *h_arg;
  int argc;
  uint8_t device_id = 0;
  char *const *argv;
  pipe_stat_tbl_hdl_t stat_tbl_hdl;
  pipe_mgr_stat_tbl_t *stat_tbl;
  static char usage[] =
      "usage :lrt evict table info for a -d <dev_id> -h "
      "<tbl_hdl>\n";
  UCLI_COMMAND_INFO(uc,
                    "lrt-evict-tbl-info",
                    -1,
                    "Show lrt evict count on the table"
                    " lrt-evict-tbl-info -d <dev_id> -h <tbl_hdl>\n");
  dflag = hflag = 0;
  d_arg = h_arg = NULL;
  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        d_arg = optarg;
        if (!d_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      case 'h':
        hflag = 1;
        h_arg = optarg;
        if (!h_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (dflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else { /*if (dflag == 1)*/
    device_id = strtoul(d_arg, NULL, 0);
    if ((device_id == 0xFF) || (device_id > (PIPE_MGR_NUM_DEVICES - 1))) {
      aim_printf(
          &uc->pvs, "lrt-evict-info : Invalid device_id %d\n", device_id);
      return UCLI_STATUS_OK;
    }
  }
  if (hflag == 0) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  } else {
    stat_tbl_hdl = strtoul(h_arg, NULL, 16);
  }

  stat_tbl = pipe_mgr_stat_tbl_get(device_id, stat_tbl_hdl);
  if (stat_tbl == NULL) {
    aim_printf(&uc->pvs, "Stats table does not exist\n");
    return UCLI_STATUS_OK;
  }
  if (stat_tbl->lrt_enabled == false) {
    aim_printf(&uc->pvs, "LRT not enabled for table 0x%x\n", stat_tbl_hdl);
    return UCLI_STATUS_OK;
  }
  aim_printf(&uc->pvs,
             "Number of LRT evicts rcvd : %" PRIu64 "\n",
             stat_tbl->num_lrt_evicts_rcvd);
  return UCLI_STATUS_OK;
}

PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(set_detail_trace) {
  extern char *optarg;
  extern int optind;
  int c, argc;
  char *e_arg = NULL;
  int enable = 0;
  char *const *argv;
  static char usage[] = "usage :detail_trace -e <enable(0/1)\n>";

  UCLI_COMMAND_INFO(uc,
                    "detail-trace",
                    -1,
                    "Enable stat mgr detailed traces"
                    " Usage : detail_trace -e <enable(0/1)>\n");

  optind = 0;
  argc = (uc->pargs->count + 1);
  argv = (char *const *)&(uc->pargs->args__);

  while ((c = getopt(argc, argv, "e:")) != -1) {
    switch (c) {
      case 'e':
        e_arg = optarg;
        if (!e_arg) {
          aim_printf(&uc->pvs, "%s", usage);
          return UCLI_STATUS_OK;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!e_arg) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  enable = strtoul(e_arg, NULL, 10);
  stat_mgr_enable_detail_trace = enable;

  return UCLI_STATUS_OK;
}
PIPE_MGR_STAT_TBL_CLI_CMD_DECLARE(show_trace) {
  PIPE_MGR_CLI_PROLOGUE("show_trace",
                        "Display per-instance trace",
                        "-d <dev> -h <table handle> -p <pipe-id>");
  bf_dev_id_t dev_id = 0;
  pipe_mat_tbl_hdl_t tbl_hdl = 0;
  bf_dev_pipe_t pipe_id = BF_DEV_PIPE_ALL;

  int c;
  while ((c = getopt(argc, argv, "d:h:p:")) != -1) {
    switch (c) {
      case 'd':
        dev_id = strtoul(optarg, NULL, 0);
        break;
      case 'h':
        tbl_hdl = strtoul(optarg, NULL, 0);
        break;
      case 'p':
        pipe_id = strtoul(optarg, NULL, 0);
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (dev_id < 0 || dev_id >= PIPE_MGR_NUM_DEVICES) {
    aim_printf(&uc->pvs, "Invalid device %d\n", dev_id);
    return UCLI_STATUS_OK;
  }

  pipe_mgr_stat_tbl_t *stat_tbl = get_stat_tbl(uc, dev_id, tbl_hdl);
  if (!stat_tbl) return UCLI_STATUS_OK;

  pipe_mgr_stat_tbl_instance_t *inst =
      pipe_mgr_stat_tbl_get_instance(stat_tbl, pipe_id);
  if (!inst) {
    aim_printf(
        &uc->pvs,
        "Cannot find stat table 0x%x instance for pipe %x on device %d\n",
        stat_tbl->stat_tbl_hdl,
        pipe_id,
        dev_id);
    return UCLI_STATUS_OK;
  }

  int buf_len = 256;
  char *buf = PIPE_MGR_MALLOC(buf_len);
  if (!buf) {
    aim_printf(&uc->pvs, "Malloc failure, %d bytes\n", buf_len);
    return UCLI_STATUS_OK;
  }
  pipe_status_t x = PIPE_SUCCESS;

  PIPE_MGR_LOCK(&inst->trace_mtx);
  for (uint32_t i = 0; i < PIPE_MGR_STAT_TRACE_SIZE; ++i) {
    uint32_t idx = (inst->trace_idx + i) & PIPE_MGR_STAT_TRACE_MASK;
    pipe_mgr_stat_tbl_trace_entry_t *t = &inst->trace[idx];
    do {
      x = pipe_mgr_stat_trace_str(t, buf, buf_len);
      if (x == PIPE_NO_SPACE) {
        PIPE_MGR_FREE(buf);
        buf_len = buf_len * 2;
        buf = PIPE_MGR_MALLOC(buf_len);
        if (!buf) {
          aim_printf(&uc->pvs, "Malloc failure, %d bytes\n", buf_len);
          PIPE_MGR_UNLOCK(&inst->trace_mtx);
          return UCLI_STATUS_OK;
        }
      }
    } while (x == PIPE_NO_SPACE && buf_len < 2048);
    if (x == PIPE_SUCCESS) {
      aim_printf(&uc->pvs, "%4d: %s\n", i, buf);
    } else if (x == PIPE_OBJ_NOT_FOUND) {
      /* Trace entry is not valid, just continue. */
    } else {
      aim_printf(&uc->pvs, "%4d: %s\n", i, pipe_str_err(x));
    }
  }
  PIPE_MGR_UNLOCK(&inst->trace_mtx);
  if (buf) PIPE_MGR_FREE(buf);
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f pipe_mgr_stat_tbl_ucli_ucli_handlers[] = {
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(tbl_info),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(entry_info),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(load_cntr),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(handle_info),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(lrt_evict_entry_info),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(lrt_evict_tbl_info),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(set_detail_trace),
    PIPE_MGR_STAT_TBL_CLI_CMD_HNDLR(show_trace),
    NULL};

static ucli_module_t pipe_mgr_stat_tbl_ucli_module = {
    "stat_tbl_ucli",
    NULL,
    pipe_mgr_stat_tbl_ucli_ucli_handlers,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_stat_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_stat_tbl_ucli_module);
  m = ucli_node_create("stat_mgr", n, &pipe_mgr_stat_tbl_ucli_module);
  ucli_node_subnode_add(m, ucli_module_log_node_create("stat_mgr"));
  return m;
}

#else

void *pipe_mgr_stat_tbl_ucli_node_create(void) { return NULL; }

#endif
