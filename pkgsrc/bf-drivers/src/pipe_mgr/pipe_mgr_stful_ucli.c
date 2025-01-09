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


/* Standard header includes */
#include <getopt.h>
#include <limits.h>

/* Module header includes */
#include <dvm/bf_drv_intf.h>
#include "lld/bf_dma_if.h"
#include <lld/lld_dr_if.h>

/* Local header includes */
#include "pipe_mgr_int.h"
#include "pipe_mgr_stful_tbl_mgr.h"

#if PIPE_MGR_CONFIG_INCLUDE_UCLI == 1

#include <target-utils/uCli/ucli.h>
#include <target-utils/uCli/ucli_argparse.h>
#include <target-utils/uCli/ucli_handler_macros.h>

#define PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(name) \
  pipe_mgr_stful_tbl_ucli_ucli__##name##__
#define PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(name)                               \
  static ucli_status_t PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(name)(ucli_context_t * \
                                                              uc)
static bool parse_param_hdl(ucli_context_t *uc,
                            char *usage,
                            pipe_reg_param_hdl_t *hdl) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *hdl = strtoul(optarg, NULL, 0);
  return false;
}

static bool parse_int64(ucli_context_t *uc, char *usage, int64_t *value) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *value = strtoll(optarg, NULL, 0);

  if ((*value > (1LL << 33) - 1) || *value < -(1LL << 33)) {
    aim_printf(&uc->pvs,
               "Invalid value provided %" PRId64
               ", must be in range for 34 bit signed.\n",
               *value);
    return true;
  }
  return false;
}

static bool parse_dev_id(ucli_context_t *uc, char *usage, bf_dev_id_t *dev_id) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *dev_id = strtoul(optarg, NULL, 0);
  if ((*dev_id < 0) || (*dev_id >= PIPE_MGR_NUM_DEVICES)) {
    aim_printf(&uc->pvs, "Invalid dev_id %d\n", *dev_id);
    return true;
  }
  return false;
}
static bool valid_pipe(bf_dev_id_t dev, int pipe) {
  if (pipe == BF_DEV_PIPE_ALL) return true;

  rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev);
  if (!dev_info) return false;
  return (pipe >= 0) && (pipe < (int)dev_info->num_active_pipes);
}
static bool parse_pipe(ucli_context_t *uc, char *usage, int *pipe) {
  extern char *optarg;
  unsigned long pipe_ul;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  pipe_ul = strtoul(optarg, NULL, 0);
  if (pipe_ul >= PIPE_MGR_MAX_PIPES && pipe_ul != BF_DEV_PIPE_ALL) {
    aim_printf(&uc->pvs, "Invalid pipe %lu\n", pipe_ul);
    return true;
  }
  *pipe = (int)pipe_ul;
  return false;
}
static bool parse_tbl_hdl(ucli_context_t *uc,
                          char *usage,
                          pipe_stful_tbl_hdl_t *h) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *h = strtoul(optarg, NULL, 0);
  if (PIPE_HDL_TYPE_STFUL_TBL != PIPE_GET_HDL_TYPE(*h) &&
      PIPE_HDL_TYPE_MAT_TBL != PIPE_GET_HDL_TYPE(*h)) {
    aim_printf(&uc->pvs, "Invalid stateful table handle %#x\n", *h);
    return true;
  }
  return false;
}
static bool parse_stful_tbl_hdl(ucli_context_t *uc,
                                char *usage,
                                pipe_stful_tbl_hdl_t *h) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *h = strtoul(optarg, NULL, 0);
  if (PIPE_HDL_TYPE_STFUL_TBL != PIPE_GET_HDL_TYPE(*h)) {
    aim_printf(&uc->pvs, "Invalid stateful table handle %#x\n", *h);
    return true;
  }
  return false;
}
static bool parse_match_tbl_hdl(ucli_context_t *uc,
                                char *usage,
                                pipe_mat_tbl_hdl_t *h) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *h = strtoul(optarg, NULL, 0);
  if (PIPE_HDL_TYPE_MAT_TBL != PIPE_GET_HDL_TYPE(*h)) {
    aim_printf(&uc->pvs, "Invalid match table handle %#x\n", *h);
    return true;
  }
  return false;
}
static bool parse_session(ucli_context_t *uc, char *usage, pipe_sess_hdl_t *h) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *h = strtoul(optarg, NULL, 0);
  if (PIPE_MGR_MAX_SESSIONS <= *h) {
    aim_printf(&uc->pvs, "Invalid session handle %#x\n", *h);
    return true;
  }
  return false;
}
static bool parse_spec(ucli_context_t *uc, char *usage, uint64_t *spec) {
  extern char *optarg;
  if (!optarg) {
    aim_printf(&uc->pvs, "%s", usage);
    return true;
  }
  *spec = strtoul(optarg, NULL, 0);
  return false;
}
static bool raw_spec_to_spec(ucli_context_t *uc,
                             bf_dev_id_t dev,
                             pipe_stful_tbl_hdl_t h,
                             uint64_t raw_spec,
                             pipe_stful_mem_spec_t *spec) {
  pipe_stful_tbl_info_t *ti;
  ti = pipe_mgr_get_stful_tbl_info(dev, h, __func__, __LINE__);
  if (!ti) {
    aim_printf(
        &uc->pvs, "Cannot find stateful table %#x on device %d\n", h, dev);
    return true;
  }
  switch (ti->width) {
    case 1:
      spec->bit = raw_spec;
      break;
    case 8:
      if (ti->dbl_width) {
        spec->dbl_byte.hi = raw_spec >> 8;
        spec->dbl_byte.lo = raw_spec & 0xFF;
      } else {
        spec->byte = raw_spec & 0xFF;
      }
      break;
    case 16:
      if (ti->dbl_width) {
        spec->dbl_half.hi = raw_spec >> 16;
        spec->dbl_half.lo = raw_spec & 0xFFFF;
      } else {
        spec->half = raw_spec & 0xFFFF;
      }
      break;
    case 32:
      if (ti->dbl_width) {
        spec->dbl_word.hi = raw_spec >> 32;
        spec->dbl_word.lo = raw_spec & 0xFFFFFFFF;
      } else {
        spec->word = raw_spec & 0xFFFFFFFF;
      }
      break;
    default:
      aim_printf(&uc->pvs,
                 "Unexpected stateful table width %d (dual=%d)\n",
                 ti->width,
                 ti->dbl_width);
      return true;
  }
  return false;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(show_stateful_tables) {
  PIPE_MGR_CLI_PROLOGUE(
      "show-stateful-tables", " Display all stateful tables", "-d <dev_id>");

  int dflag = 0;
  bf_dev_id_t dev_id = 0;

  int c;
  while ((c = getopt(argc, argv, "d:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  bool loop = true;
  for (; dev_id < PIPE_MGR_NUM_DEVICES && loop; ++dev_id) {
    loop = !dflag;
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
    if (!dev_info) continue;
    unsigned int p = 0;
    for (; p < dev_info->num_pipeline_profiles; ++p) {
      rmt_dev_tbl_info_t *ti = &dev_info->profile_info[p]->tbl_info_list;
      if (ti->num_sful_tbls) {
        aim_printf(&uc->pvs, "Device %d\n", dev_id);
      }

      unsigned int t = 0, m;
      for (; t < ti->num_sful_tbls; ++t) {
        pipe_stful_tbl_info_t *info = &ti->stful_tbl_list[t];
        aim_printf(&uc->pvs,
                   "%#x width %d bit%s %d entries, %s\n",
                   info->handle,
                   info->width,
                   info->dbl_width ? "x2" : "",
                   info->size,
                   info->name);
        for (m = 0; m < ti->num_mat_tbls; ++m) {
          unsigned int r = 0;
          for (; r < ti->mat_tbl_list[m].num_stful_tbl_refs; ++r) {
            if (ti->mat_tbl_list[m].stful_tbl_ref[r].tbl_hdl != info->handle)
              continue;
            aim_printf(&uc->pvs,
                       "\t   %sirect-ref by %s %#x\n",
                       ti->mat_tbl_list[m].stful_tbl_ref[r].ref_type ==
                               PIPE_TBL_REF_TYPE_DIRECT
                           ? "D"
                           : "Ind",
                       ti->mat_tbl_list[m].name,
                       ti->mat_tbl_list[m].handle);
          }
        }
      }
    }

    if (dflag) break;
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(stateful_table_info) {
  PIPE_MGR_CLI_PROLOGUE("stateful-table-info",
                        " Display details of a stateful table",
                        "-d <dev_id> -h <stateful table handle>");

  int dflag = 0, hflag = 0;
  bf_dev_id_t dev_id = 0;
  pipe_stful_tbl_hdl_t h = 0;

  int c;
  while ((c = getopt(argc, argv, "d:h:")) != -1) {
    switch (c) {
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_stful_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  bool dloop = true;
  for (; dev_id < PIPE_MGR_NUM_DEVICES && dloop; ++dev_id) {
    dloop = !dflag;
    rmt_dev_info_t *dev_info = pipe_mgr_get_dev_info(dev_id);
    if (!dev_info) continue;
    unsigned int p = 0;
    for (; p < dev_info->num_pipeline_profiles; ++p) {
      rmt_dev_tbl_info_t *ti = &dev_info->profile_info[p]->tbl_info_list;

      unsigned int t = 0;
      for (; t < ti->num_sful_tbls; ++t) {
        pipe_stful_tbl_info_t *info = &ti->stful_tbl_list[t];
        if (hflag && h != info->handle) continue;

        size_t buf_sz = 4096;
        char *buf = PIPE_MGR_CALLOC(buf_sz, 1);
        if (!buf) {
          aim_printf(&uc->pvs, "Failed to allocate memory for dump\n");
          return UCLI_STATUS_OK;
        }

        int sts = pipe_mgr_stful_log_tbl_info_to_buf(
            dev_id, info->handle, buf, buf_sz);
        if (sts) {
          aim_printf(&uc->pvs,
                     "Dump failed for dev %d tbl %#x\n",
                     dev_id,
                     info->handle);
          return UCLI_STATUS_OK;
        } else {
          aim_printf(&uc->pvs, "%s", buf);
        }

        PIPE_MGR_FREE(buf);
      }
    }

    if (dflag) break;
  }

  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(stateful_table_reset) {
  PIPE_MGR_CLI_PROLOGUE("stateful-table-reset",
                        " Write the entire table",
                        "-s <session_hld> -d <dev_id> -h <stateful table "
                        "handle> -p <pipe> [-v <spec>]");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0, vflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_sess_hdl_t shdl = 0;
  pipe_stful_tbl_hdl_t h = 0;
  uint64_t raw_spec = 0;
  pipe_stful_mem_spec_t spec = {0};
  pipe_stful_mem_spec_t *spec_p = NULL;

  int c;
  while ((c = getopt(argc, argv, "s:d:h:p:v:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_stful_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'v':
        vflag = 1;
        if (parse_spec(uc, usage, &raw_spec)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!sflag || !dflag || !hflag || !pflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }
  if (vflag) {
    spec_p = &spec;
    if (raw_spec_to_spec(uc, dev_id, h, raw_spec, spec_p))
      return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_table_reset(shdl, dt, h, spec_p);
  if (vflag) {
    aim_printf(&uc->pvs,
               "Table reset by session %#x on dev %d pipe %d to table %#x with "
               "spec 0x%08" PRIx64 "%08" PRIx64 " returned status %s\n",
               shdl,
               dev_id,
               pipe,
               h,
               raw_spec >> 32,
               raw_spec & 0xFFFFFFFF,
               pipe_str_err(sts));
  } else {
    aim_printf(&uc->pvs,
               "Table reset by session %#x on dev %d pipe %d to table %#x "
               "returned status %s\n",
               shdl,
               dev_id,
               pipe,
               h,
               pipe_str_err(sts));
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(stateful_table_range_reset) {
  PIPE_MGR_CLI_PROLOGUE("stateful-table-range-reset",
                        " Write part of a table",
                        "-s <session_hld> -d <dev_id> -h <stateful table "
                        "handle> -p <pipe> -b <begin_index> -c <count> [-v "
                        "<spec>]");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0, vflag = 0, bflag = 0,
      cflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_sess_hdl_t shdl = 0;
  pipe_stful_tbl_hdl_t h = 0;
  uint64_t raw_spec = 0;
  pipe_stful_mem_spec_t spec = {0};
  pipe_stful_mem_spec_t *spec_p = NULL;
  int index = 0, count = 0;

  int c;
  while ((c = getopt(argc, argv, "s:d:h:p:v:b:c:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_stful_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'v':
        vflag = 1;
        if (parse_spec(uc, usage, &raw_spec)) return UCLI_STATUS_OK;
        break;
      case 'b':
        bflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return true;
        }
        index = strtoul(optarg, NULL, 0);
        if (index < 0) {
          aim_printf(&uc->pvs, "%s", usage);
          return true;
        }
        break;
      case 'c':
        cflag = 1;
        if (!optarg) {
          aim_printf(&uc->pvs, "%s", usage);
          return true;
        }
        count = strtoul(optarg, NULL, 0);
        if (count <= 0) {
          aim_printf(&uc->pvs, "%s", usage);
          return true;
        }
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }
  if (!sflag || !dflag || !hflag || !pflag || !bflag || !cflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }
  if (vflag) {
    spec_p = &spec;
    if (raw_spec_to_spec(uc, dev_id, h, raw_spec, spec_p))
      return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_table_reset_range(shdl, dt, h, index, count, spec_p);
  if (vflag) {
    aim_printf(&uc->pvs,
               "Table reset by session %#x on dev %d pipe %d to table %#x "
               "%d-%d with spec 0x%08" PRIx64 "%08" PRIx64
               " returned status %s\n",
               shdl,
               dev_id,
               pipe,
               h,
               index,
               index + count - 1,
               raw_spec >> 32,
               raw_spec & 0xFFFFFFFF,
               pipe_str_err(sts));
  } else {
    aim_printf(&uc->pvs,
               "Table reset by session %#x on dev %d pipe %d to table %#x "
               "%d-%d returned status %s\n",
               shdl,
               dev_id,
               pipe,
               h,
               index,
               index + count - 1,
               pipe_str_err(sts));
  }
  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(stateful_table_sync) {
  PIPE_MGR_CLI_PROLOGUE(
      "stateful-table-sync",
      " Read the contents of a stateful table from HW",
      "-s <session_hld> -d <dev_id> -p <pipe> -h <stateful table handle>");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_stful_tbl_hdl_t h = 0;
  pipe_sess_hdl_t shdl = 0;

  int c;
  while ((c = getopt(argc, argv, "s:d:p:h:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_stful_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!sflag || !dflag || !pflag || !hflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_database_sync(shdl, dt, h, NULL, NULL);

  aim_printf(&uc->pvs,
             "Table sync by session %#x on dev %d pipe %d to table %#x "
             "returned status %s\n",
             shdl,
             dev_id,
             pipe,
             h,
             pipe_str_err(sts));
  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(stateful_table_direct_sync) {
  PIPE_MGR_CLI_PROLOGUE(
      "stateful-table-direct-sync",
      " Read the contents of a stateful table from HW",
      "-s <session_hld> -d <dev_id> -p <pipe> -h <match table handle>");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_mat_tbl_hdl_t h = 0;
  pipe_sess_hdl_t shdl = 0;

  int c;
  while ((c = getopt(argc, argv, "s:d:p:h:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_match_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!sflag || !dflag || !pflag || !hflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_direct_database_sync(shdl, dt, h, NULL, NULL);

  aim_printf(&uc->pvs,
             "Table sync by session %#x on dev %d pipe %d to table %#x "
             "returned status %s\n",
             shdl,
             dev_id,
             pipe,
             h,
             pipe_str_err(sts));
  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(param_set) {
  PIPE_MGR_CLI_PROLOGUE("param-set",
                        " Set register param related to table by name",
                        "-s <session_hld> -d <dev_id> -p <pipe> -h <match "
                        "table handle> -n <register param handle> -m <value>");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0, nflag = 0, mflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_mat_tbl_hdl_t h = 0;
  pipe_reg_param_hdl_t rph = 0;
  pipe_sess_hdl_t shdl = 0;
  int64_t value = 0;

  int c;
  while ((c = getopt(argc, argv, "s:d:p:h:n:m:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      case 'n':
        nflag = 1;
        if (parse_param_hdl(uc, usage, &rph)) return UCLI_STATUS_OK;
        break;
      case 'm':
        mflag = 1;
        if (parse_int64(uc, usage, &value)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!sflag || !dflag || !pflag || !hflag || !nflag || !mflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_param_set(shdl, dt, h, rph, value);

  aim_printf(&uc->pvs, "Completed: err %s\n", pipe_str_err(sts));
  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(param_get) {
  PIPE_MGR_CLI_PROLOGUE("param-get",
                        " Set register param related to table by name",
                        "-s <session_hld> -d <dev_id> -p <pipe> -h <match "
                        "table handle> -n <register param handle>");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0, nflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_mat_tbl_hdl_t h = 0;
  pipe_reg_param_hdl_t rph = 0;
  pipe_sess_hdl_t shdl = 0;
  int64_t value;

  int c;
  while ((c = getopt(argc, argv, "s:d:p:h:n:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      case 'n':
        nflag = 1;
        if (parse_param_hdl(uc, usage, &rph)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!sflag || !dflag || !pflag || !hflag || !nflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_param_get(shdl, dt, h, rph, &value);
  if (sts == PIPE_SUCCESS) {
    aim_printf(&uc->pvs, "%d: %" PRId64 "\n", rph, value);
  }

  aim_printf(&uc->pvs, "Completed: err %s\n", pipe_str_err(sts));
  return UCLI_STATUS_OK;
}

PIPE_MGR_STFUL_TBL_CLI_CMD_DECLARE(param_reset) {
  PIPE_MGR_CLI_PROLOGUE("param-reset",
                        " Reset register param related to table by name",
                        "-s <session_hld> -d <dev_id> -p <pipe> -h <match "
                        "table handle> -n <register param handle>");

  int dflag = 0, hflag = 0, pflag = 0, sflag = 0, nflag = 0;
  bf_dev_id_t dev_id = 0;
  int pipe = 0;
  pipe_mat_tbl_hdl_t h = 0;
  pipe_reg_param_hdl_t rph = 0;
  pipe_sess_hdl_t shdl = 0;

  int c;
  while ((c = getopt(argc, argv, "s:d:p:h:n:")) != -1) {
    switch (c) {
      case 's':
        sflag = 1;
        if (parse_session(uc, usage, &shdl)) return UCLI_STATUS_OK;
        break;
      case 'd':
        dflag = 1;
        if (parse_dev_id(uc, usage, &dev_id)) return UCLI_STATUS_OK;
        break;
      case 'p':
        pflag = 1;
        if (parse_pipe(uc, usage, &pipe)) return UCLI_STATUS_OK;
        break;
      case 'h':
        hflag = 1;
        if (parse_tbl_hdl(uc, usage, &h)) return UCLI_STATUS_OK;
        break;
      case 'n':
        nflag = 1;
        if (parse_param_hdl(uc, usage, &rph)) return UCLI_STATUS_OK;
        break;
      default:
        aim_printf(&uc->pvs, "%s", usage);
        return UCLI_STATUS_OK;
    }
  }

  if (!sflag || !dflag || !pflag || !hflag || !nflag) {
    aim_printf(&uc->pvs, "%s", usage);
    return UCLI_STATUS_OK;
  }
  if (!valid_pipe(dev_id, pipe)) {
    aim_printf(&uc->pvs, "Invalid pipe %d\n", pipe);
    return UCLI_STATUS_OK;
  }

  pipe_status_t sts;
  dev_target_t dt = {.device_id = dev_id, .dev_pipe_id = pipe};
  sts = pipe_stful_param_reset(shdl, dt, h, rph);

  aim_printf(&uc->pvs, "Completed: err %s\n", pipe_str_err(sts));
  return UCLI_STATUS_OK;
}

static ucli_command_handler_f pipe_mgr_stful_tbl_ucli_ucli_handlers__[] = {
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(show_stateful_tables),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_table_info),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_table_reset),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_table_range_reset),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_table_sync),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_table_direct_sync),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(param_set),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(param_get),
    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(param_reset),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(get_full_instr_ptr),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(get_entry_vaddr),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(get_entry_vaddr_direct),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(run_alu),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_entry_write),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(stateful_entry_write_direct),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(sync_index),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(sync_entry_direct),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(read_entry_shadow),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(read_entry_shadow_direct),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(read_entry_hw),
    //    PIPE_MGR_STFUL_TBL_CLI_CMD_HNDLR(read_entry_hw_direct),
    NULL};

static ucli_module_t pipe_mgr_stful_tbl_ucli_module__ = {
    "stful-tbl-ucli",
    NULL,
    pipe_mgr_stful_tbl_ucli_ucli_handlers__,
    NULL,
    NULL,
};

ucli_node_t *pipe_mgr_stful_tbl_ucli_node_create(ucli_node_t *n) {
  ucli_node_t *m;
  ucli_module_init(&pipe_mgr_stful_tbl_ucli_module__);
  m = ucli_node_create("stful-mgr", n, &pipe_mgr_stful_tbl_ucli_module__);
  ucli_node_subnode_add(m, ucli_module_log_node_create("stful-mgr"));
  return m;
}

#else

void *pipe_mgr_stful_tbl_ucli_node_create(void) { return NULL; }

#endif
