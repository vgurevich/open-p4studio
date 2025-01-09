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
 * @file pipe_mgr_int.h
 * @date
 *
 * Internal definitions for pipe_mgr.
 */
#ifndef __PIPE_MGR_INT_H__
#define __PIPE_MGR_INT_H__

#ifdef PIPE_MGR_THREADS
#include <target-sys/bf_sal/bf_sys_sem.h>
#endif

#include <pipe_mgr/pipe_mgr_config.h>

#include <target-sys/bf_sal/bf_sys_str.h>
#include <target-utils/map/map.h>
#include <target-utils/list/bf_list.h>
#include <target-utils/bitset/bitset.h>
#include <target-utils/hashtbl/bf_hashtbl.h>
#include <target-utils/third-party/cJSON/cJSON.h>
#include <bf_types/bf_types.h>
#include <lld/lld_reg_if.h>

#define PIPE_MGR_LOGICAL_ACT_IDX_INVALID 0xdeadbeef
#define PIPE_MGR_TBL_NO_KEY_DEFAULT_ENTRY_HDL 1

#define TOF_BYTES_IN_RAM_WORD 16
#define TOF_MAX_RAM_WORDS_IN_ADT_TBL_WORD 8
#define TOF_MAX_EXM_WAYS_PER_P4_TABLE (40)
#define TOF_MAX_EXM_SUBENTRIES_PER_P4_TABLE (40)
#define TOF_MAX_GFM_COLUMNS (52)
#define TOF_MAX_HASH_GROUPS (2)
#define TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD 8

#define PIPE_MGR_TBL_MAX_LOG_IDS 50
typedef uint8_t exm_tbl_word_t[TOF_MAX_RAM_WORDS_IN_EXM_TBL_WORD]
                              [TOF_BYTES_IN_RAM_WORD];

typedef struct pipe_register_spec {
  uint32_t reg_addr;
  uint32_t reg_data;
} pipe_register_spec_t;

typedef struct pipe_memory_spec {
  uint64_t mem_addr0;
  uint64_t mem_addr1;
} pipe_memory_spec_t;

#define TOF_MAX_TCAM_WORDS_IN_TERN_TBL_WORD 16
#define TOF_BYTES_IN_TCAM_WORD 6
#define TOF_BYTES_IN_TCAM_WHOLE_WORD 16

typedef uint8_t tern_tbl_word_t[TOF_MAX_TCAM_WORDS_IN_TERN_TBL_WORD]
                               [TOF_BYTES_IN_TCAM_WORD];
typedef uint8_t tind_tbl_word_t[TOF_BYTES_IN_RAM_WORD];
typedef uint8_t adt_tbl_word_t[TOF_MAX_RAM_WORDS_IN_ADT_TBL_WORD]
                              [TOF_BYTES_IN_RAM_WORD];

#include <pipe_mgr/pipe_mgr_intf.h>

#define UNUSED(x) (void)x;

#include "pipe_mgr_mutex.h"
#include "pipe_mgr_log.h"
#include "pipe_mgr_session_int.h"
#include "pipe_mgr_rmt_cfg.h"
#include "pipe_mgr_pktgen.h"
#include "pipe_mgr_table_packing.h"
#include "pipe_mgr_tofino_cfg.h"
#include "pipe_mgr_tof2_cfg.h"
#include "pipe_mgr_tof3_cfg.h"

#include "pipe_mgr_parde.h"

#define PIPE_MGR_TBL_API_TXN (1 << 0)
#define PIPE_MGR_TBL_API_ATOM (1 << 1)
#define PIPE_MGR_TBL_API_CACHE_ENT_ID PIPE_FLAG_CACHE_ENT_ID
/* Flag to enable skip backup in unwanted transactions */
#define PIPE_FLAG_SKIP_BACKUP (1 << 3)
#define PIPE_MGR_TBL_API_CONST_ENT (1 << 4)

#define PIPE_MGR_PORT_TO_PIPE_SHIFT 7

#define PIPE_BMP_SIZE PIPE_MGR_MAX_PIPES
#define PIPE_MGR_PROFILE_COUNT 4

#define PIPE_MGR_INVALID_MOVEREG_ADDR ((unsigned)~0)

#define PIPE_MGR_MAU_WORD_WIDTH TOF_BYTES_IN_RAM_WORD

typedef uint16_t lock_id_t;

bool pipe_mgr_valid_deviceId(bf_dev_id_t devId,
                             const char *where,
                             const int line);
bool pipe_mgr_valid_dev_tgt(bf_dev_target_t dt,
                            const char *where,
                            const int line);
pipe_status_t pipe_mgr_abort_txn_int(pipe_sess_hdl_t shdl);

#define PIPE_MGR_CLI_PROLOGUE(CMD, HELP, USAGE)                            \
  extern char *optarg;                                                     \
  extern int optind;                                                       \
  UCLI_COMMAND_INFO(uc, CMD, -1, HELP " Usage: " CMD " " USAGE);           \
  char usage[] = "Usage: " USAGE "\n";                                     \
  int arg_start = 0;                                                       \
  {                                                                        \
    size_t i;                                                              \
    for (i = 0;                                                            \
         i < sizeof(uc->pargs[0].args__) / sizeof(uc->pargs[0].args__[0]); \
         ++i) {                                                            \
      if (!strncmp(uc->pargs[0].args__[i], CMD, strlen(CMD))) {            \
        arg_start = i;                                                     \
        break;                                                             \
      }                                                                    \
    }                                                                      \
  }                                                                        \
  int argc = uc->pargs->count + 1;                                         \
  if (1 == argc) {                                                         \
    aim_printf(&uc->pvs, "Usage: " USAGE "\n");                            \
    return UCLI_STATUS_OK;                                                 \
  }                                                                        \
  char *const *argv = (char *const *)&(uc->pargs->args__[arg_start]);      \
  optind = 0;

//
// Simple double linked list macros.  Head's prev points to tail.  Tail's
// next is NULL.
//

// Prepend.
#define PIPE_MGR_DLL_PP(l, e, n, p) BF_LIST_DLL_PP(l, e, n, p)
// Get last.
#define PIPE_MGR_DLL_LAST(l, r, n, p) BF_LIST_DLL_LAST(l, r, n, p)
// Append.
#define PIPE_MGR_DLL_AP(l, e, n, p) BF_LIST_DLL_AP(l, e, n, p)
// Remove.
#define PIPE_MGR_DLL_REM(l, e, n, p) BF_LIST_DLL_REM(l, e, n, p)

//
// Container macro.  Given a member of a struct, return a pointer to the
// containing struct.
#define PIPE_MGR_GET_CONTAINER(ptrToMbr, mbr, type) \
  ((ptrToMbr) ? ((type *)((char *)(ptrToMbr)-offsetof(type, mbr))) : NULL)

// Some looping macros for Judy

#define JUDYL_FOREACH(__PJLArray__, __Index, __Val_type, __Val)          \
  PWord_t __PValue_p;                                                    \
  Word_t __Index_p = 0;                                                  \
  for (__PValue_p = (Pvoid_t)JudyLFirst(__PJLArray__, &__Index_p, PJE0), \
      (__PValue_p ? (__Val = (__Val_type)*__PValue_p) : (__Val = 0)),    \
      (__Index = __Index_p);                                             \
       __Val;                                                            \
       __PValue_p = (Pvoid_t)JudyLNext(__PJLArray__, &__Index_p, PJE0),  \
      (__PValue_p ? (__Val = (__Val_type)*__PValue_p) : (__Val = 0)),    \
      (__Index = __Index_p))

/* This is a pretty inelegant solution to a dumb problem.  When building with
 * -Wshadow you can't use JUDYL_FOREACH in a double for loop because the macro
 * define the same temporary variables again.  So rather than disabling the
 * warning for these few lines of code we just make a second version of the
 * macro with new names. */
#define JUDYL_FOREACH2(__PJLArray__, __Index, __Val_type, __Val, n)            \
  PWord_t __PValue_p##n;                                                       \
  Word_t __Index_p##n = 0;                                                     \
  for (__PValue_p##n = (Pvoid_t)JudyLFirst(__PJLArray__, &__Index_p##n, PJE0), \
      (__PValue_p##n ? (__Val = (__Val_type)*__PValue_p##n) : (__Val = 0)),    \
      (__Index = __Index_p##n);                                                \
       __Val;                                                                  \
       __PValue_p##n = (Pvoid_t)JudyLNext(__PJLArray__, &__Index_p##n, PJE0),  \
      (__PValue_p##n ? (__Val = (__Val_type)*__PValue_p##n) : (__Val = 0)),    \
      (__Index = __Index_p##n))

#define JUDY1_FOREACH(__PJ1Array__, __Index)                  \
  int __Rc_int;                                               \
  Word_t __Index_p = 0;                                       \
  for (__Rc_int = Judy1First(__PJ1Array__, &__Index_p, PJE0), \
      (__Index = __Index_p);                                  \
       __Rc_int;                                              \
       __Rc_int = Judy1Next(__PJ1Array__, &__Index_p, PJE0),  \
      (__Index = __Index_p))

typedef enum pipe_mgr_lock_id_type_e {
  LOCK_ID_TYPE_INVALID = 0,
  LOCK_ID_TYPE_IDLE_BARRIER,
  LOCK_ID_TYPE_STAT_BARRIER,
  LOCK_ID_TYPE_IDLE_LOCK,
  LOCK_ID_TYPE_STAT_LOCK,
  LOCK_ID_TYPE_ALL_LOCK,
} pipe_mgr_lock_id_type_e;

#define PIPE_MGR_INVALID_LOCK_ID 0
#define PIPE_MGR_GET_LOCK_ID_TYPE(x) (x >> 12)
#define PIPE_MGR_FORM_LOCK_ID(l, t, x) ((l) = ((t) << 12) | ((x)&0xfff))
#define PIPE_MGR_GET_ID_FROM_LOCK_ID(l) (l & 0xfff)

#define PIPE_MGR_HW_DUMP_STR_LEN 21500

#define RMT_EXM_ENTRY_VERSION_INVALID 0
#define RMT_EXM_ENTRY_VERSION_OLD 1
#define RMT_EXM_ENTRY_VERSION_NEW 2
#define RMT_EXM_ENTRY_VERSION_DONT_CARE 3

#define RMT_EXM_SET_ENTRY_VERSION_VALID_BITS(__a__, __b__, __c__)             \
  {                                                                           \
    bool __bit0__ = 0;                                                        \
    bool __bit1__ = 0;                                                        \
    bool __bit2__ = 0;                                                        \
    bool __bit3__ = 0;                                                        \
    if (__a__ == RMT_EXM_ENTRY_VERSION_NEW ||                                 \
        __a__ == RMT_EXM_ENTRY_VERSION_DONT_CARE) {                           \
      __bit0__ = 1;                                                           \
    }                                                                         \
    if (__b__ == RMT_EXM_ENTRY_VERSION_NEW ||                                 \
        __b__ == RMT_EXM_ENTRY_VERSION_DONT_CARE) {                           \
      __bit1__ = 1;                                                           \
    }                                                                         \
    if (__a__ == RMT_EXM_ENTRY_VERSION_OLD ||                                 \
        __a__ == RMT_EXM_ENTRY_VERSION_DONT_CARE) {                           \
      __bit2__ = 1;                                                           \
    }                                                                         \
    if (__b__ == RMT_EXM_ENTRY_VERSION_OLD ||                                 \
        __b__ == RMT_EXM_ENTRY_VERSION_DONT_CARE) {                           \
      __bit3__ = 1;                                                           \
    }                                                                         \
    __c__ = (__bit3__ << 3) | (__bit2__ << 2) | (__bit1__ << 1) | (__bit0__); \
  }

/* Global maps from <tbl_hdl> to table information. */
typedef struct pipe_mgr_map_ctx_s {
  bf_map_t tbl_hdl_to_tbl_map;
  /* Backup table map */
  bf_map_t tbl_hdl_to_btbl_map;
} pipe_mgr_map_ctx_t;

struct pipe_mgr_dev_ctx {
  bf_dev_init_mode_t dev_init_mode;
  bool warm_init_in_progress;
  bf_map_t sm_tbl_db;
  pipe_mgr_sm_tbl_info_t *sm_tbl_info;
  /* Provides exclusive access to the Session Manager's table database. */
  pipe_mgr_rmutex_t sm_tbl_mtx;

  /* Which session handle is currently updating the packet generator. */
  pipe_sess_hdl_t pkt_gen;

  /* An array sized by the number of mirror session indicating which session
   * handle is currently updating each mirror session. */
  pipe_mgr_sm_mir_info_t *mir_ses;

  /* Length of the mir_ses array. */
  unsigned int mir_ses_length;

  /* Which session handle is currently updating the dynamic hash calc. */
  pipe_sess_hdl_t hash_calc;

  /* Which session handle is currently updating the learn notifications */
  pipe_sess_hdl_t learn_ses;

  /* Packet Generator Context. */
  struct pipe_mgr_pg_dev_ctx *pgc;

  /* Stateful Memory Management Context. */
  bf_map_t stful_tbls;

  /* Provides exclusive access to the Stateful table map. */
  pipe_mgr_mutex_t stful_tbl_mtx;

  /* Maps a <tbl_hdl> to Phase0 table information. Generally just one but
   * potentially one per pipe if multiple profiles are used. */
  bf_map_t p0_tbls;

  /* Provides exclusive access to the phase0 table map. */
  pipe_mgr_mutex_t p0_tbl_mtx;

  /* Maps a pipe+stage+logical-table-id to a table handle.  Used when
   * processing stats messages coming from HW to map the pipe/stage/logical
   * table id to a stats table handle. */
  bf_map_t stat_tbl_hdls;

  /* A global map from <tbl_hdl> to stat table information. */
  bf_map_t stat_tbl_map;

  /* Provides exclusive access to the stat table map. */
  pipe_mgr_mutex_t stat_tbl_mtx;

  /* A global map from <tbl_hdl> to meter table information. */
  bf_map_t meter_tbl_map;

  /* Provides exclusive access to the meter table map. */
  pipe_mgr_mutex_t meter_tbl_mtx;

  /* A global map from <tbl_hdl> to action data table information. */
  bf_map_t adt_tbl_map;

  /* Provides exclusive access to the action data table map. */
  pipe_mgr_mutex_t adt_tbl_mtx;

  /* Global maps from <tbl_hdl> to selector table information. */
  pipe_mgr_map_ctx_t sel_ctx;

  /* Provides exclusive access to the selector table maps. */
  pipe_mgr_mutex_t sel_tbl_mtx;

  /* Global maps from <tbl_hdl> to TCAM table information. */
  pipe_mgr_map_ctx_t tcam_ctx;

  /* Provides exclusive access to the TCAM table maps. */
  pipe_mgr_mutex_t tcam_tbl_mtx;

  /* Provides exclusive access to the idle table map. */
  pipe_mgr_mutex_t idle_tbl_mtx;

  /* Global maps from <tbl_hdl> to exact match table information. */
  bf_map_t exm_tbl_map;

  /* Provides exclusive access to the exact match table map. */
  pipe_mgr_mutex_t exm_tbl_mtx;

  /* Global maps from <tbl_hdl> to DKM match table information. */
  bf_map_t dkm_tbl_map;
  pipe_mgr_mutex_t dkm_tbl_mtx;

  /* Maps a dev_port to a boolean indicating if overspeed mode is requested on
   * that port. */
  bf_map_t overspeed_25g_map;
  pipe_mgr_mutex_t overspeed_25g_mtx;

  /* Maintains state about ebuf configuration. */
  union pipe_mgr_ebuf_ctx ebuf_ctx;

  /* Maintains state about deparser configuration. */
  union pipe_mgr_deprsr_ctx deprsr_ctx;

  /* Flag to log instruction list writes. */
  int log_ilist;
};

/* Global context for pipe_mgr service. */
typedef struct pipe_mgr_ctx {
  /* Session context blocks. */
  pipe_mgr_sess_ctx_t pipe_mgr_sessions[PIPE_MGR_MAX_SESSIONS];

  /* Internal session handle. */
  pipe_sess_hdl_t int_ses_hndl;

  /* Protects the pipe_mgr_sessions[] array. */
  pipe_mgr_mutex_t ses_list_mtx;

  /* Memory allocation and deallocation functions to use for driver-specific
   * placement data. May be null, in which case bf_sys_malloc and bf_sys_free
   * will be used instead. */
  pipe_plcmnt_alloc alloc_fn;
  pipe_plcmnt_free free_fn;

  /* RW lock used to create cases where only a single API to the driver is
   * allowed.  For example, the remove device API cannot handle other APIs such
   * as table adds running in parallel because the thread calling remove device
   * will be freeing data structures that the thread doing table-add is reading.
   */
  pipe_mgr_rw_mutex_lock_t api_lock;

  /* Maps a device id to a rmt_dev_info_t* */
  bf_map_t dev_info_map;

  /* Maps a bf_dev_id_t to a struct pipe_mgr_dev_ctx. */
  bf_map_t dev_ctx;

  /* If enabled adt and selector will cache entry id, otherwise argument will be
   * ignored. Used by bf-rt. */
  bool cache_ent_id;

  uint32_t pipe_mgr_submodule_debug;
} pipe_mgr_ctx_t;

typedef struct pipe_mgr_stage_char {
  bool match_dp;
  int clock_cycles;
} pipe_mgr_stage_char_t;

struct pipe_mgr_ctx *get_pipe_mgr_ctx();

static inline void pipe_mgr_dev_ctx_set(bf_dev_id_t dev,
                                        struct pipe_mgr_dev_ctx *ctx) {
  unsigned long key = dev;
  bf_map_add(&get_pipe_mgr_ctx()->dev_ctx, key, ctx);
}
static inline void pipe_mgr_dev_ctx_rmv(bf_dev_id_t dev) {
  unsigned long key = dev;
  struct pipe_mgr_dev_ctx *ctx = NULL;
  bf_map_get_rmv(&get_pipe_mgr_ctx()->dev_ctx, key, (void **)&ctx);
  if (ctx) {
    bf_map_destroy(&ctx->stat_tbl_hdls);
    bf_map_destroy(&ctx->stat_tbl_map);
    bf_map_destroy(&ctx->stful_tbls);
    bf_map_destroy(&ctx->p0_tbls);
    bf_map_destroy(&ctx->meter_tbl_map);
    bf_map_destroy(&ctx->adt_tbl_map);
    bf_map_destroy(&ctx->sel_ctx.tbl_hdl_to_tbl_map);
    bf_map_destroy(&ctx->sel_ctx.tbl_hdl_to_btbl_map);
    bf_map_destroy(&ctx->tcam_ctx.tbl_hdl_to_tbl_map);
    bf_map_destroy(&ctx->tcam_ctx.tbl_hdl_to_btbl_map);
    bf_map_destroy(&ctx->exm_tbl_map);
    bf_map_destroy(&ctx->dkm_tbl_map);
    bf_map_destroy(&ctx->overspeed_25g_map);
    PIPE_MGR_LOCK_DESTROY(&ctx->stat_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->stful_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->p0_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->meter_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->adt_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->sel_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->tcam_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->idle_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->exm_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->dkm_tbl_mtx);
    PIPE_MGR_LOCK_DESTROY(&ctx->overspeed_25g_mtx);
    PIPE_MGR_FREE(ctx);
  }
}
static inline struct pipe_mgr_dev_ctx *pipe_mgr_dev_ctx(bf_dev_id_t dev) {
  void *dev_ctx = NULL;
  unsigned long key = dev;
  bf_map_sts_t s = bf_map_get(&get_pipe_mgr_ctx()->dev_ctx, key, &dev_ctx);
  if (BF_MAP_OK == s) return dev_ctx;
  return NULL;
  // return &get_pipe_mgr_ctx()->dev_ctx[dev];
}
static inline struct pipe_mgr_dev_ctx *pipe_mgr_dev_ctx_(bf_dev_id_t dev) {
  // return get_pipe_mgr_ctx()->dev_ctx[dev].device_present
  //           ? &get_pipe_mgr_ctx()->dev_ctx[dev]
  //           : NULL;
  return pipe_mgr_dev_ctx(dev);
}
static inline rmt_dev_info_t *pipe_mgr_get_dev_info(bf_dev_id_t dev_id) {
  struct pipe_mgr_ctx *c = get_pipe_mgr_ctx();
  if (!c) return NULL;
  bf_map_t *m = &c->dev_info_map;
  rmt_dev_info_t *d = NULL;
  bf_map_sts_t s = bf_map_get(m, dev_id, (void **)&d);
  return BF_MAP_OK == s ? d : NULL;
}

static inline int pipe_mgr_num_prsrs(bf_dev_id_t dev_id) {
  rmt_dev_info_t *info = pipe_mgr_get_dev_info(dev_id);
  return info ? info->dev_cfg.num_prsrs : -1;
}

static inline bf_map_t *pipe_mgr_sm_tbl_db(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  return ctx ? &ctx->sm_tbl_db : NULL;
}
static inline pipe_mgr_sm_tbl_info_t **pipe_mgr_sm_tbl_info(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  return ctx ? &ctx->sm_tbl_info : NULL;
}
static inline pipe_sess_hdl_t pipe_mgr_get_pkt_gen_hdl(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return 0;
  }
  return ctx->pkt_gen;
}
static inline void pipe_mgr_set_pkt_gen_hdl(bf_dev_id_t dev,
                                            pipe_sess_hdl_t hdl) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->pkt_gen = hdl;
}
static inline pipe_sess_hdl_t pipe_mgr_get_hash_calc_hdl(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return 0;
  }
  return ctx->hash_calc;
}
static inline void pipe_mgr_set_hash_calc_hdl(bf_dev_id_t dev,
                                              pipe_sess_hdl_t hdl) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->hash_calc = hdl;
}
static inline pipe_mgr_sm_mir_info_t **pipe_mgr_mir_ses_hdl_array_get(
    bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  return ctx ? &ctx->mir_ses : NULL;
}
static inline void pipe_mgr_set_mir_ses_hdl_array_len(bf_dev_id_t dev,
                                                      unsigned int len) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (ctx) ctx->mir_ses_length = len;
}
static inline pipe_status_t pipe_mgr_get_mir_ses_hdl(bf_dev_id_t dev,
                                                     bf_mirror_id_t sid,
                                                     pipe_sess_hdl_t *shdl) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return PIPE_UNEXPECTED;
  }

  if (sid >= ctx->mir_ses_length) {
    return PIPE_INVALID_ARG;
  }

  *shdl = ctx->mir_ses[sid].sid;
  return PIPE_SUCCESS;
}
static inline pipe_status_t pipe_mgr_set_mir_ses_hdl(bf_dev_id_t dev,
                                                     bf_mirror_id_t mid,
                                                     pipe_sess_hdl_t shdl) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return PIPE_UNEXPECTED;
  }
  if (mid >= ctx->mir_ses_length) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_INVALID_ARG;
  }

  pipe_mgr_sess_ctx_t *s = pipe_mgr_get_sess_ctx(shdl, __func__, __LINE__);
  if (s == NULL) {
    PIPE_MGR_DBGCHK(0);
    return PIPE_UNEXPECTED;
  }
  ctx->mir_ses[mid].sid = shdl;
  PIPE_MGR_DLL_PP(s->mir_ses[dev], &ctx->mir_ses[mid], next, prev);
  return PIPE_SUCCESS;
}
static inline pipe_sess_hdl_t pipe_mgr_get_lrn_ses_hdl(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return 0;
  }
  return ctx->learn_ses;
}
static inline void pipe_mgr_set_lrn_ses_hdl(bf_dev_id_t dev,
                                            pipe_sess_hdl_t hdl) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->learn_ses = hdl;
}
static inline union pipe_mgr_ebuf_ctx *pipe_mgr_ebuf_ctx(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *d = pipe_mgr_dev_ctx(dev);
  return d ? &d->ebuf_ctx : NULL;
}
static inline union pipe_mgr_deprsr_ctx *pipe_mgr_deprsr_ctx(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev);
  return ctx ? &ctx->deprsr_ctx : NULL;
}
static inline int pipe_mgr_log_ilist(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev);
  return ctx ? ctx->log_ilist : 0;
}
static inline void pipe_mgr_set_log_ilist(bf_dev_id_t dev, int x) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->log_ilist = x;
}
static inline pipe_mgr_rmutex_t *pipe_mgr_smtbl_mtx(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  return ctx ? &ctx->sm_tbl_mtx : NULL;
}

static inline bf_map_sts_t pipe_mgr_meter_tbl_map_add(bf_dev_id_t dev,
                                                      unsigned long key,
                                                      void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->meter_tbl_mtx);
  map_sts = bf_map_add(&ctx->meter_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->meter_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_meter_tbl_map_rmv(bf_dev_id_t dev,
                                                      unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->meter_tbl_mtx);
  map_sts = bf_map_rmv(&ctx->meter_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->meter_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_meter_tbl_map_get(bf_dev_id_t dev,
                                                      unsigned long key,
                                                      void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->meter_tbl_mtx);
  map_sts = bf_map_get(&ctx->meter_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->meter_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_meter_tbl_map_get_first(bf_dev_id_t dev,
                                                            unsigned long *key,
                                                            void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->meter_tbl_mtx);
  map_sts = bf_map_get_first(&ctx->meter_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->meter_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_meter_tbl_map_get_next(bf_dev_id_t dev,
                                                           unsigned long *key,
                                                           void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->meter_tbl_mtx);
  map_sts = bf_map_get_next(&ctx->meter_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->meter_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stat_tbl_map_add(bf_dev_id_t dev,
                                                     unsigned long key,
                                                     void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stat_tbl_mtx);
  map_sts = bf_map_add(&ctx->stat_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stat_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stat_tbl_map_rmv(bf_dev_id_t dev,
                                                     unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stat_tbl_mtx);
  map_sts = bf_map_rmv(&ctx->stat_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->stat_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stat_tbl_map_get(bf_dev_id_t dev,
                                                     unsigned long key,
                                                     void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stat_tbl_mtx);
  map_sts = bf_map_get(&ctx->stat_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stat_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stat_tbl_hdls_map_add(bf_dev_id_t dev,
                                                          unsigned long key,
                                                          void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stat_tbl_mtx);
  /* Remove the mapping if it already exists. */
  bf_map_rmv(&ctx->stat_tbl_hdls, key);
  /* Add the new mapping. */
  map_sts = bf_map_add(&ctx->stat_tbl_hdls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stat_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stat_tbl_hdls_map_get(bf_dev_id_t dev,
                                                          unsigned long key,
                                                          void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stat_tbl_mtx);
  map_sts = bf_map_get(&ctx->stat_tbl_hdls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stat_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_p0_tbl_map_add(bf_dev_id_t dev,
                                                   unsigned long key,
                                                   void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->p0_tbl_mtx);
  map_sts = bf_map_add(&ctx->p0_tbls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->p0_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_p0_tbl_map_rmv(bf_dev_id_t dev,
                                                   unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->p0_tbl_mtx);
  map_sts = bf_map_rmv(&ctx->p0_tbls, key);
  PIPE_MGR_UNLOCK(&ctx->p0_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_p0_tbl_map_get(bf_dev_id_t dev,
                                                   unsigned long key,
                                                   void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->p0_tbl_mtx);
  map_sts = bf_map_get(&ctx->p0_tbls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->p0_tbl_mtx);
  return map_sts;
}
static inline bf_map_sts_t pipe_mgr_stful_tbl_map_add(bf_dev_id_t dev,
                                                      unsigned long key,
                                                      void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stful_tbl_mtx);
  map_sts = bf_map_add(&ctx->stful_tbls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stful_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stful_tbl_map_get_rmv(bf_dev_id_t dev,
                                                          unsigned long key,
                                                          void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stful_tbl_mtx);
  map_sts = bf_map_get_rmv(&ctx->stful_tbls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stful_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_stful_tbl_map_get(bf_dev_id_t dev,
                                                      unsigned long key,
                                                      void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->stful_tbl_mtx);
  map_sts = bf_map_get(&ctx->stful_tbls, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->stful_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_adt_tbl_map_add(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->adt_tbl_mtx);
  map_sts = bf_map_add(&ctx->adt_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->adt_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_adt_tbl_map_rmv(bf_dev_id_t dev,
                                                    unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->adt_tbl_mtx);
  map_sts = bf_map_rmv(&ctx->adt_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->adt_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_adt_tbl_map_get(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->adt_tbl_mtx);
  map_sts = bf_map_get(&ctx->adt_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->adt_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_sel_tbl_map_add(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void *tbl,
                                                    bool is_backup) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->sel_tbl_mtx);
  if (is_backup)
    map_sts = bf_map_add(&ctx->sel_ctx.tbl_hdl_to_btbl_map, key, tbl);
  else
    map_sts = bf_map_add(&ctx->sel_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->sel_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_sel_tbl_map_rmv(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    bool is_backup) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->sel_tbl_mtx);
  if (is_backup)
    map_sts = bf_map_rmv(&ctx->sel_ctx.tbl_hdl_to_btbl_map, key);
  else
    map_sts = bf_map_rmv(&ctx->sel_ctx.tbl_hdl_to_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->sel_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_sel_tbl_map_get(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void **tbl,
                                                    bool is_backup) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->sel_tbl_mtx);
  if (is_backup)
    map_sts = bf_map_get(&ctx->sel_ctx.tbl_hdl_to_btbl_map, key, tbl);
  else
    map_sts = bf_map_get(&ctx->sel_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->sel_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_sel_tbl_map_get_first(bf_dev_id_t dev,
                                                          unsigned long *key,
                                                          void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->sel_tbl_mtx);
  map_sts = bf_map_get_first(&ctx->sel_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->sel_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_sel_tbl_map_get_next(bf_dev_id_t dev,
                                                         unsigned long *key,
                                                         void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->sel_tbl_mtx);
  map_sts = bf_map_get_next(&ctx->sel_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->sel_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_tcam_tbl_map_add(bf_dev_id_t dev,
                                                     unsigned long key,
                                                     void *tbl,
                                                     bool is_backup) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->tcam_tbl_mtx);
  if (is_backup)
    map_sts = bf_map_add(&ctx->tcam_ctx.tbl_hdl_to_btbl_map, key, tbl);
  else
    map_sts = bf_map_add(&ctx->tcam_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->tcam_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_tcam_tbl_map_rmv(bf_dev_id_t dev,
                                                     unsigned long key,
                                                     bool is_backup) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->tcam_tbl_mtx);
  if (is_backup)
    map_sts = bf_map_rmv(&ctx->tcam_ctx.tbl_hdl_to_btbl_map, key);
  else
    map_sts = bf_map_rmv(&ctx->tcam_ctx.tbl_hdl_to_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->tcam_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_tcam_tbl_map_get(bf_dev_id_t dev,
                                                     unsigned long key,
                                                     void **tbl,
                                                     bool is_backup) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->tcam_tbl_mtx);
  if (is_backup)
    map_sts = bf_map_get(&ctx->tcam_ctx.tbl_hdl_to_btbl_map, key, tbl);
  else
    map_sts = bf_map_get(&ctx->tcam_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->tcam_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_tcam_tbl_map_get_first(bf_dev_id_t dev,
                                                           unsigned long *key,
                                                           void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->tcam_tbl_mtx);
  map_sts = bf_map_get_first(&ctx->tcam_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->tcam_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_tcam_tbl_map_get_next(bf_dev_id_t dev,
                                                          unsigned long *key,
                                                          void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->tcam_tbl_mtx);
  map_sts = bf_map_get_next(&ctx->tcam_ctx.tbl_hdl_to_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->tcam_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_idle_tbl_map_add(bf_dev_id_t dev,
                                                     bf_map_t *map,
                                                     unsigned long key,
                                                     void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->idle_tbl_mtx);
  map_sts = bf_map_add(map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->idle_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_idle_tbl_map_rmv(bf_dev_id_t dev,
                                                     bf_map_t *map,
                                                     unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->idle_tbl_mtx);
  map_sts = bf_map_rmv(map, key);
  PIPE_MGR_UNLOCK(&ctx->idle_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_idle_tbl_map_get(bf_dev_id_t dev,
                                                     bf_map_t *map,
                                                     unsigned long key,
                                                     void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->idle_tbl_mtx);
  map_sts = bf_map_get(map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->idle_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_idle_tbl_map_get_first(bf_dev_id_t dev,
                                                           bf_map_t *map,
                                                           unsigned long *key,
                                                           void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->idle_tbl_mtx);
  map_sts = bf_map_get_first(map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->idle_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_idle_tbl_map_get_next(bf_dev_id_t dev,
                                                          bf_map_t *map,
                                                          unsigned long *key,
                                                          void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->idle_tbl_mtx);
  map_sts = bf_map_get_next(map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->idle_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_exm_tbl_map_add(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->exm_tbl_mtx);
  map_sts = bf_map_add(&ctx->exm_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->exm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_exm_tbl_map_rmv(bf_dev_id_t dev,
                                                    unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->exm_tbl_mtx);
  map_sts = bf_map_rmv(&ctx->exm_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->exm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_exm_tbl_map_get(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->exm_tbl_mtx);
  map_sts = bf_map_get(&ctx->exm_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->exm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_dkm_tbl_map_add(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void *tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->dkm_tbl_mtx);
  map_sts = bf_map_add(&ctx->dkm_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->dkm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_dkm_tbl_map_rmv(bf_dev_id_t dev,
                                                    unsigned long key) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->dkm_tbl_mtx);
  map_sts = bf_map_rmv(&ctx->dkm_tbl_map, key);
  PIPE_MGR_UNLOCK(&ctx->dkm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_dkm_tbl_map_get(bf_dev_id_t dev,
                                                    unsigned long key,
                                                    void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->dkm_tbl_mtx);
  map_sts = bf_map_get(&ctx->dkm_tbl_map, key, tbl);
  PIPE_MGR_UNLOCK(&ctx->dkm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_dkm_tbl_map_get_first_rmv(bf_dev_id_t dev,
                                                              void **tbl) {
  struct pipe_mgr_dev_ctx *ctx;
  unsigned long key = 0;
  bf_map_sts_t map_sts;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->dkm_tbl_mtx);
  map_sts = bf_map_get_first_rmv(&ctx->dkm_tbl_map, &key, tbl);
  PIPE_MGR_UNLOCK(&ctx->dkm_tbl_mtx);
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_overspeed_map_add(bf_dev_id_t dev,
                                                      bf_dev_port_t dev_port) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;
  unsigned long key = dev_port;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->overspeed_25g_mtx);
  map_sts = bf_map_add(&ctx->overspeed_25g_map, key, (void *)(uintptr_t)1);
  PIPE_MGR_UNLOCK(&ctx->overspeed_25g_mtx);

  if (map_sts == BF_MAP_OK || map_sts == BF_MAP_KEY_EXISTS) {
    return BF_MAP_OK;
  } else {
    return map_sts;
  }
}

static inline bf_map_sts_t pipe_mgr_overspeed_map_rmv(bf_dev_id_t dev,
                                                      bf_dev_port_t dev_port) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;
  unsigned long key = dev_port;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->overspeed_25g_mtx);
  map_sts = bf_map_rmv(&ctx->overspeed_25g_map, key);
  PIPE_MGR_UNLOCK(&ctx->overspeed_25g_mtx);

  if (map_sts == BF_MAP_OK || map_sts == BF_MAP_NO_KEY) {
    return BF_MAP_OK;
  } else {
    return map_sts;
  }
}

static inline bf_map_sts_t pipe_mgr_overspeed_map_get(bf_dev_id_t dev,
                                                      bf_dev_port_t dev_port,
                                                      bool *enabled) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;
  unsigned long key = dev_port;
  void *val = NULL;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->overspeed_25g_mtx);
  map_sts = bf_map_get(&ctx->overspeed_25g_map, key, &val);
  PIPE_MGR_UNLOCK(&ctx->overspeed_25g_mtx);

  /* If it is present in the map it means it is enabled, not present means it is
   * disabled. */
  if (map_sts == BF_MAP_OK) {
    *enabled = true;
    return BF_MAP_OK;
  } else if (map_sts == BF_MAP_NO_KEY) {
    *enabled = false;
    return BF_MAP_OK;
  } else {
    *enabled = false;
    return map_sts;
  }
}

static inline bf_map_sts_t pipe_mgr_overspeed_map_get_first(
    bf_dev_id_t dev, bf_dev_port_t *dev_port) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;
  unsigned long key = 0;
  void *val = NULL;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->overspeed_25g_mtx);
  map_sts = bf_map_get_first(&ctx->overspeed_25g_map, &key, &val);
  PIPE_MGR_UNLOCK(&ctx->overspeed_25g_mtx);

  if (map_sts == BF_MAP_OK) *dev_port = key;
  return map_sts;
}

static inline bf_map_sts_t pipe_mgr_overspeed_map_get_next(
    bf_dev_id_t dev, bf_dev_port_t *dev_port) {
  struct pipe_mgr_dev_ctx *ctx;
  bf_map_sts_t map_sts;
  unsigned long key = *dev_port;
  void *val = NULL;

  ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) return BF_MAP_ERR;

  PIPE_MGR_LOCK(&ctx->overspeed_25g_mtx);
  map_sts = bf_map_get_next(&ctx->overspeed_25g_map, &key, &val);
  PIPE_MGR_UNLOCK(&ctx->overspeed_25g_mtx);

  if (map_sts == BF_MAP_OK) *dev_port = key;
  return map_sts;
}

static inline struct pipe_mgr_pg_dev_ctx *pipe_mgr_pktgen_ctx(bf_dev_id_t dev) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  return ctx ? ctx->pgc : NULL;
}
static inline void pipe_mgr_pktgen_ctx_set(bf_dev_id_t dev,
                                           struct pipe_mgr_pg_dev_ctx *c) {
  struct pipe_mgr_dev_ctx *ctx = pipe_mgr_dev_ctx_(dev);
  if (!ctx) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  ctx->pgc = c;
}
static inline void pipe_mgr_lock_mau_scratch(bf_dev_id_t dev) {
  rmt_dev_info_t *info = pipe_mgr_get_dev_info(dev);
  if (!info) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  PIPE_MGR_LOCK(&info->mau_scratch_mtx);
}
static inline void pipe_mgr_unlock_mau_scratch(bf_dev_id_t dev) {
  rmt_dev_info_t *info = pipe_mgr_get_dev_info(dev);
  if (!info) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  PIPE_MGR_UNLOCK(&info->mau_scratch_mtx);
}
static inline uint32_t pipe_mgr_get_mau_scratch_val(bf_dev_id_t dev,
                                                    bf_dev_pipe_t pipe,
                                                    uint32_t stage) {
  rmt_dev_info_t *info = pipe_mgr_get_dev_info(dev);
  if (!info) {
    PIPE_MGR_ASSERT(0);
    return 0;
  }
  return info->mau_scratch_val[pipe][stage];
}
static inline void pipe_mgr_set_mau_scratch_val(bf_dev_id_t dev,
                                                bf_dev_pipe_t pipe,
                                                uint32_t stage,
                                                uint32_t val) {
  rmt_dev_info_t *info = pipe_mgr_get_dev_info(dev);
  if (!info) {
    PIPE_MGR_ASSERT(0);
    return;
  }
  info->mau_scratch_val[pipe][stage] = val;
}
static inline bool pipe_mgr_device_present(bf_dev_id_t dev) {
  return !!pipe_mgr_dev_ctx(dev);
}

#include "pipe_mgr_instr.h"
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>

/* Internal API only. Has no locking and other checks */
/* Given a match table and a match entry handle, returns the pipe,
 * stage, and the logical entry index of associated direct addressed
 * entries such as action data, stats, idle time, meters etc.
 */
pipe_status_t pipe_mgr_mat_ent_get_dir_ent_location(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    uint32_t subindex,
    bf_dev_pipe_t *pipe_id_p,
    dev_stage_t *stage_id_p,
    rmt_tbl_hdl_t *stage_table_hdl_p,
    uint32_t *index_p);

/* Internal API only. Has no locking and other checks */
/* Given a match table and a match entry handle, returns
 * a reference to the corresponding match_spec_t
 */
pipe_status_t pipe_mgr_ent_hdl_to_match_spec_internal(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_mat_ent_hdl_t mat_ent_hdl,
    bf_dev_pipe_t *ent_pipe_id,
    pipe_tbl_match_spec_t const **match_spec_out);

/* Internal API only. Has no locking and other checks */
pipe_status_t pipe_mgr_mat_tbl_gen_lock_id(bf_dev_id_t device_id,
                                           pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                           pipe_mgr_lock_id_type_e lock_id_type,
                                           lock_id_t *lock_id_p);

/* Internal API only. Has no locking and other checks */
pipe_status_t pipe_mgr_mat_tbl_update_lock_type(bf_dev_id_t dev_id,
                                                pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                                bool idle,
                                                bool stat,
                                                bool add_lock);

/* Internal API only. Has no locking and other checks */
pipe_status_t pipe_mgr_mat_tbl_update_idle_init_val(
    bf_dev_id_t dev_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    uint32_t idle_init_val_for_ttl_0);

pipe_status_t pipe_mgr_mat_tbl_reset_idle(pipe_sess_hdl_t sess_hdl,
                                          bf_dev_id_t device_id,
                                          pipe_mat_tbl_hdl_t mat_tbl_hdl,
                                          bf_dev_pipe_t pipe_id,
                                          uint8_t stage_id,
                                          mem_id_t mem_id,
                                          uint32_t mem_offset);

/* Internal API only. Has no locking and other checks */
pipe_status_t pipe_mgr_mat_tbl_get_dir_stat_tbl_hdl(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_stat_tbl_hdl_t *stat_tbl_hdl_p);

pipe_status_t pipe_mgr_mat_tbl_get_dir_meter_tbl_hdl(
    bf_dev_id_t device_id,
    pipe_mat_tbl_hdl_t mat_tbl_hdl,
    pipe_meter_tbl_hdl_t *meter_tbl_hdl_p);

bf_status_t pipe_mgr_lock_device(bf_dev_id_t dev_id);

bf_status_t pipe_mgr_unlock_device_cb(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_unlock_device_internal(bf_dev_id_t dev_id,
                                            bf_dev_init_mode_t warm_init_mode);
bf_status_t pipe_mgr_unlock_device_cleanup(bf_dev_id_t dev_id);

bf_status_t pipe_mgr_reconfig_create_dma(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_disable_traffic(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_enable_traffic(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_wait_for_traffic_flush(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_config_complete(bf_dev_id_t dev_id);
bf_status_t pipe_mgr_reconfig_error_cleanup(bf_dev_id_t dev_id);

bool pipe_mgr_is_device_locked(bf_dev_id_t dev_id);
bool pipe_mgr_is_any_device_locked();
pipe_status_t pipe_mgr_set_mem_slow_mode(rmt_dev_info_t *dev_info, bool enable);
pipe_status_t pipe_mgr_hw_notify_cfg(rmt_dev_info_t *dev_info);
pipe_status_t pipe_mgr_internal_session_push(bf_dev_id_t dev_id, bool reconfig);

void pipe_mgr_enable_all_dr(rmt_dev_info_t *dev_info);

void pipe_mgr_disable_all_dr(bf_dev_id_t dev_id);

void pipe_mgr_init_mode_set(bf_dev_id_t dev_id,
                            bf_dev_init_mode_t dev_init_mode);

void pipe_mgr_init_mode_reset(bf_dev_id_t dev_id);

bool pipe_mgr_warm_init_in_progress(bf_dev_id_t dev_id);

bool pipe_mgr_hitless_warm_init_in_progress(bf_dev_id_t dev_id);

bool pipe_mgr_fast_recfg_warm_init_in_progress(bf_dev_id_t dev_id);

bool pipe_mgr_init_mode_cold_boot(bf_dev_id_t dev_id);

bool pipe_mgr_init_mode_hitless(bf_dev_id_t dev_id);

bool pipe_mgr_init_mode_fast_recfg(bf_dev_id_t dev_id);

bool pipe_mgr_init_mode_fast_recfg_quick(bf_dev_id_t dev_id);

bf_subdev_id_t pipe_mgr_subdev_id_from_pipe(bf_dev_pipe_t pipe);

void resource_trace(bf_dev_id_t dev,
                    pipe_res_spec_t *rspecs,
                    int rsc_count,
                    char *buf,
                    size_t buf_size);

void pipe_mgr_set_pbus_weights(rmt_dev_info_t *dev_info, int weight);

ucli_status_t packet_path_ucli_show_tech_drivers(ucli_context_t *uc);

pipe_status_t pipe_mgr_get_pipe_id(pipe_bitmap_t *pipe_bmp,
                                   bf_dev_pipe_t dev_pipe_id,
                                   bf_dev_pipe_t default_pipe_id,
                                   bf_dev_pipe_t *pipe_id);

#endif /* __PIPE_MGR_INT_H__ */
