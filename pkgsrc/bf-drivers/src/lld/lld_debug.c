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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <regex.h>

#include <target-sys/bf_sal/bf_sys_intf.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dma_if.h>
#include <lld/bf_dev_if.h>
#include <lld/lld_dr_if.h>
#include <lld/lld_sku.h>
#include <lld/lld_fault.h>
#include <lld/lld_dr_regs.h>
#include <lld/lld_dr_regs_tof.h>
#include <lld/bf_dma_dr_id.h>
#include "lld.h"
#include "lld_dev.h"
#include "lld_map.h"
#include "lld_log.h"
#include "lld_memory_mapping.h"
#include <lld/python_shell_mutex.h>

extern FILE *reg_get_outstream();

// mutex to protect python cli used by lld-python and bf-python
py_shell_context_t py_shell_ctx;

typedef struct lld_log_cfg_t {
  uint8_t glbl_logs_en;
  uint8_t chip_logs_en[BF_MAX_DEV_COUNT];
  uint8_t mac_logs_en[BF_MAX_DEV_COUNT][BF_PIPE_COUNT * BF_PIPE_PORT_COUNT];
  uint8_t port_logs_en[BF_MAX_DEV_COUNT][BF_PIPE_COUNT * BF_PIPE_PORT_COUNT];
  uint8_t dma_logs_en[BF_MAX_DEV_COUNT][BF_DMA_MAX_DR];
} lld_log_cfg_t;

lld_log_cfg_t lld_log_cfg;

static char *dr_e_to_str(bf_dma_dr_id_t dr_e) {
  static char *dr_e_2_str[] = {
#define LLD_DR_ID(x) #x
      BF_DMA_DR_IDS};
  return (dr_e_2_str[dr_e]);
}
uint32_t dr_str_to_e(const char *dr_str) {
  for (int i = 0; i < BF_DMA_MAX_DR; ++i) {
    if (!strcmp(dr_str, dr_e_to_str(i))) return i;
  }
  return -1;
}

bf_sys_mutex_t lld_log_mtx;  // lock for main LLD log
int lld_log_mtx_initd = 0;

void lld_log_mtx_init(void) {
  if (!lld_log_mtx_initd) {
    int x;

    lld_log_mtx_initd = 1;

    x = bf_sys_mutex_init(&lld_log_mtx);
    if (x) {
      printf("Error: LLD log lock init failed: <%d>\n", x);
      bf_sys_assert(0);
    } else {
      lld_log_mtx_initd = 1;
    }
  }
}

void lld_debug_init(void) {
  lld_log_mtx_init();
  lld_dma_log_init();
  // Initialize the python lock
  INIT_PYTHON_SHL_LOCK();
}

void lld_dump_timeval(struct timeval *tm) {
  char tbuf[256] = {0};
  char ubuf[256] = {0};
  struct tm *loctime;

  loctime = localtime(&tm->tv_sec);

  if (loctime != NULL) {
    strftime(tbuf, sizeof(tbuf), "%a %b %d", loctime);
    printf("%s ", tbuf);

    strftime(ubuf, sizeof(ubuf), "%T\n", loctime);
    ubuf[strlen(ubuf) - 1] = 0;  // remove CR
    printf("%s.%06d", ubuf, (int)tm->tv_usec);
  }
  return;
}

typedef struct lld_dma_log_entry_t {
  int dir;  // 0=push, 1=pull, 2=start, 3=service, 4=DMA wr, 5=DMA rd
  bf_dev_id_t dev_id;
  bf_subdev_id_t subdev_id;
  uint32_t dr;
  uint32_t n_wds;
  uint64_t data[4];
  uint64_t head;
  uint64_t tail;
  struct timeval tm;
} lld_dma_log_entry_t;

#define LLD_DMA_LOG_SZ 1024

typedef struct lld_dma_log_t {
  int next;
  lld_dma_log_entry_t entry[LLD_DMA_LOG_SZ];
} lld_dma_log_t;

bf_sys_mutex_t lld_dma_log_mtx;  // lock for log
lld_dma_log_t lld_dma_log;

void lld_dma_log_init(void) {
  int x;

  x = bf_sys_mutex_init(&lld_dma_log_mtx);
  if (x) {
    printf("Error: DMA Log lock init failed: <%d>\n", x);
  }
}

/* 0=push
 * 1=pull
 * 2=start
 * 3=service
 * 4=DMA Wr
 * 5=DMA Rd
 */
void lld_log_dma(int dir,
                 bf_dev_id_t dev_id,
                 bf_subdev_id_t subdev_id,
                 uint32_t dr,
                 uint64_t *data,
                 int n_wds,
                 uint64_t head,
                 uint64_t tail) {
  int e;
  int x;
  // lock entry determination
  x = bf_sys_mutex_lock(&lld_dma_log_mtx);
  if (x) {
    lld_log_error("Error: DMA Log lock acquire failed: <%d>", x);
  }

  e = lld_dma_log.next++;
  if (lld_dma_log.next == LLD_DMA_LOG_SZ) {
    lld_dma_log.next = 0;
  }
  x = bf_sys_mutex_unlock(&lld_dma_log_mtx);
  if (x) {
    lld_log_error("Error: DMA Log lock release failed: <%d>", x);
  }

  gettimeofday(&lld_dma_log.entry[e].tm, NULL);

  lld_dma_log.entry[e].dir = dir;
  lld_dma_log.entry[e].dev_id = dev_id;
  lld_dma_log.entry[e].subdev_id = subdev_id;
  lld_dma_log.entry[e].dr = dr;
  lld_dma_log.entry[e].n_wds = n_wds;
  lld_dma_log.entry[e].head = head;
  lld_dma_log.entry[e].tail = tail;
  if ((dir == 0) || (dir == 1) || (dir == 4) || (dir == 5)) {
    lld_dma_log.entry[e].data[0] = *(data + 0ull);
    if (n_wds >= 2) {
      lld_dma_log.entry[e].data[1] = *(data + 1ull);
    }
    if (n_wds >= 4) {
      lld_dma_log.entry[e].data[2] = *(data + 2ull);
      lld_dma_log.entry[e].data[3] = *(data + 3ull);
    }
  }
}

// TBD need to pass subdev_id, work with dru_sim on this
void lld_log_dma_op(bf_dev_id_t dev_id, int is_wr, int len, uint64_t addr) {
  uint64_t tmp[4];
  uint8_t *data = (uint8_t *)lld_u64_to_void_ptr(addr);
  int i;

  for (i = 0; i < 4; i++) {
    tmp[i] = 0ull;
  }
  tmp[3] = addr;
  memcpy((char *)&tmp[0], data, (len <= 24) ? len : 24);
  if (is_wr) {
    lld_log_dma(4 /*wr*/, dev_id, 0, len /*dr*/, tmp, 4 /*n_wds*/, 0, 0);
  } else {
    lld_log_dma(5 /*rd*/, dev_id, 0, len /*dr*/, tmp, 4 /*n_wds*/, 0, 0);
  }
}

#define LLD_DMA_LOG_DUMP_HELPER(snprintf_fn)                                \
  do {                                                                      \
    x = snprintf_fn;                                                        \
    if (x < 0) { /* Unexpected error. */                                    \
      goto problem;                                                         \
    } else if (x >= (str_len - str_pos)) { /* Need more buffer, realloc. */ \
      str_len += 2048;                                                      \
      char *temp = bf_sys_realloc(buf, str_len);                            \
      if (!temp) goto problem;                                              \
      buf = temp;                                                           \
    } else { /* Success! */                                                 \
      str_pos += x;                                                         \
      break;                                                                \
    }                                                                       \
  } while (true)

char *lld_get_dma_log(bf_dev_id_t dev_id,
                      bf_subdev_id_t subdev_id,
                      uint32_t *drs_to_log,
                      int num_drs_to_log) {
  int str_len = LLD_DMA_LOG_SZ * 64;
  int str_pos = 0, x, e, n;
  char *buf = bf_sys_malloc(str_len);
  if (!buf) return NULL;

  LLD_DMA_LOG_DUMP_HELPER(snprintf(
      buf,
      str_len - str_pos,
      "%s",
      "                           |Type|Chp|   Word 0 (64b)            "
      "      Word 1 (64b)\n"
      "+--------------------------+----+---+----------+----------+-----"
      "-----+--------------\n"));

  e = lld_dma_log.next;
  for (n = 0; n < LLD_DMA_LOG_SZ; n++) {
    lld_dma_log_entry_t *p_ent = &lld_dma_log.entry[e];
    // skip empty entires
    if (p_ent->tm.tv_sec == 0) {
      e = (e == (LLD_DMA_LOG_SZ - 1)) ? 0 : (e + 1);
      continue;
    } else {
      /* Filter entries by device/subdev id and by the list of interesting DRs.
       */
      if (p_ent->dev_id != dev_id || p_ent->subdev_id != subdev_id) {
        e = (e == (LLD_DMA_LOG_SZ - 1)) ? 0 : (e + 1);
        continue;
      }
      bool log_it = num_drs_to_log == 0 || drs_to_log == NULL;
      for (int d = 0; drs_to_log && d < num_drs_to_log; ++d) {
        if (p_ent->dr == drs_to_log[d]) {
          log_it = true;
          break;
        }
      }
      if (!log_it) {
        e = (e == (LLD_DMA_LOG_SZ - 1)) ? 0 : (e + 1);
        continue;
      }

      char tbuf[256] = {0};
      char ubuf[256] = {0};
      struct tm *loctime;

      loctime = localtime(&p_ent->tm.tv_sec);

      if (loctime != NULL) {
        strftime(tbuf, sizeof(tbuf), "%a %b %d", loctime);
        LLD_DMA_LOG_DUMP_HELPER(
            snprintf(buf + str_pos, str_len - str_pos, "%s", tbuf));

        strftime(ubuf, sizeof(ubuf), "%T\n", loctime);
        ubuf[strlen(ubuf) - 1] = 0;  // remove CR
        LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                         str_len - str_pos,
                                         " %s.%06d | ",
                                         ubuf,
                                         (int)p_ent->tm.tv_usec));
      }
    }

    const char *op =
        p_ent->dir == 0
            ? "psh"
            : p_ent->dir == 1
                  ? "pul"
                  : p_ent->dir == 2
                        ? "str"
                        : p_ent->dir == 3
                              ? "srv"
                              : p_ent->dir == 4
                                    ? "dma"
                                    : p_ent->dir == 5 ? "dma" : "inv";
    LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                     str_len - str_pos,
                                     "%d | %d | %s | ",
                                     p_ent->dev_id,
                                     p_ent->subdev_id,
                                     op));

    if (p_ent->dir == 2) {  // start
      LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                       str_len - str_pos,
                                       ": DMA Start : head=%d : tail=%d : ",
                                       (p_ent->n_wds >> 16),
                                       (p_ent->n_wds & 0xffff)));
    } else if (p_ent->dir == 3) {
      LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                       str_len - str_pos,
                                       ": DMA Service : head=%d : tail=%d : ",
                                       (p_ent->n_wds >> 16),
                                       (p_ent->n_wds & 0xffff)));
    } else {
      int n_wds = p_ent->n_wds, dma_len;
      uint8_t *d = (uint8_t *)&p_ent->data[0];

      if ((p_ent->dir == 4) || (p_ent->dir == 5)) {
        LLD_DMA_LOG_DUMP_HELPER(
            snprintf(buf + str_pos,
                     str_len - str_pos,
                     ": DMA %s : %d : %016" PRIx64
                     "\n                                     | ",
                     (p_ent->dir == 4) ? "Wr" : "Rd",
                     p_ent->dr,
                     p_ent->data[3]));
        dma_len = p_ent->dr < 24 ? p_ent->dr : 24;
        for (n_wds = 0; n_wds < dma_len; n_wds++) {
          LLD_DMA_LOG_DUMP_HELPER(snprintf(
              buf + str_pos, str_len - str_pos, "%02x ", *(d + n_wds)));
        }
        if (dma_len < (int)p_ent->dr) {
          LLD_DMA_LOG_DUMP_HELPER(
              snprintf(buf + str_pos, str_len - str_pos, ".."));
        }
      } else if (n_wds == 1) {
        LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                         str_len - str_pos,
                                         "%016" PRIx64 "",
                                         p_ent->data[0]));
      } else if (n_wds == 2) {
        LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                         str_len - str_pos,
                                         "%016" PRIx64 "  %016" PRIx64 "",
                                         p_ent->data[0],
                                         p_ent->data[1]));
      } else if (n_wds == 3) {
        LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                         str_len - str_pos,
                                         "%016" PRIx64 "  %016" PRIx64
                                         "  %016" PRIx64 "",
                                         p_ent->data[0],
                                         p_ent->data[1],
                                         p_ent->data[2]));
      } else if (n_wds == 4) {
        LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                         str_len - str_pos,
                                         "%016" PRIx64 "  %016" PRIx64
                                         "  %016" PRIx64 "  %016" PRIx64 "",
                                         p_ent->data[0],
                                         p_ent->data[1],
                                         p_ent->data[2],
                                         p_ent->data[3]));
      }
    }
    if ((p_ent->dir == 4) || (p_ent->dir == 5)) {
      LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos, str_len - str_pos, "\n"));
    } else {
      LLD_DMA_LOG_DUMP_HELPER(snprintf(buf + str_pos,
                                       str_len - str_pos,
                                       " : %s  head=%" PRIu64 " tail=%" PRIu64
                                       "\n",
                                       dr_e_to_str(p_ent->dr),
                                       p_ent->head,
                                       p_ent->tail));
    }
    e = (e == (LLD_DMA_LOG_SZ - 1)) ? 0 : (e + 1);
  }
  return buf;
problem:
  bf_sys_free(buf);
  return NULL;
}

void lld_print_dma_log(bf_dev_id_t dev_id,
                       bf_subdev_id_t subdev_id,
                       uint32_t *drs_to_log,
                       int num_drs_to_log) {
  char *log_str =
      lld_get_dma_log(dev_id, subdev_id, drs_to_log, num_drs_to_log);
  if (log_str) {
    fprintf(reg_get_outstream(), "\n%s\n", log_str);
    bf_sys_free(log_str);
  }
}

void lld_dump_dr(bf_dev_id_t dev_id,
                 bf_subdev_id_t subdev_id,
                 bf_dma_dr_id_t dr) {
  lld_dr_view_t *view = lld_map_subdev_id_and_dr_to_view(dev_id, subdev_id, dr);
  uint32_t hd_ptr, hd_wrp, tl_ptr, tl_wrp;

  if (!view) {
    fprintf(reg_get_outstream(), "%s:%d View not found", __func__, __LINE__);
    return;
  }

  dr_update_view(view);

  hd_ptr = TOF_DR_PTR_PART(view->head);
  hd_wrp = TOF_DR_WRP_PART(view->head);
  tl_ptr = TOF_DR_PTR_PART(view->tail);
  tl_wrp = TOF_DR_WRP_PART(view->tail);

  fprintf(reg_get_outstream(),
          "%2d| %d |%4d| %d |%4d|%4d| %d | %d | %06" PRIx64 " | %016" PRIx64
          " | %016" PRIx64 " | %4d | %s\n",
          dr,
          hd_wrp,
          hd_ptr,
          tl_wrp,
          tl_ptr,
          view->n_entries,
          view->n_words_per_desc,
          view->lock_reqd,
          view->last_pointer_written & 0xffffff,
          (uint64_t)lld_ptr_to_u64(view->dev_pushed_ptr),
          view->base,
          dr_used(view),
          dr_e_to_str(dr));
}

void lld_dump_drs(void) {
  int i, j;
  int depth;
  bf_dma_dr_id_t d;
  bf_dma_dr_id_t dr_max;
  fprintf(reg_get_outstream(), "\nLLD DR Views:\n");
  for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
    if (!lld_dev_ready(i, 0)) continue;
    dr_max = lld_dr_get_max_dr(i, 0);
    fprintf(
        reg_get_outstream(),
        "--+--------+--------+----+---+---+--------+------------------+--------"
        "----------+------+----------------\n");
    fprintf(
        reg_get_outstream(),
        "dr| head   |   tail | sz |wds| lk|last ptr|   limit          |   base "
        "          | used | name\n");
    fprintf(
        reg_get_outstream(),
        "--+--------+--------+----+---+---+--------+------------------+--------"
        "----------+------+----------------\n");
    for (j = 0; j < BF_MAX_SUBDEV_COUNT; j++) {
      for (d = 0; d <= dr_max; d++) {
        uint32_t dru_base = lld_dr_base_get(i, d);

        /* do not access the DRs that are not implemented on this chip */
        if (dru_base == 0) {
          continue;
        }
        /* do not access the DRs that are not managed by this process */
        if (lld_dr_cfg_depth_get(i, j, d, &depth) != LLD_OK) continue;
        if (depth == 0) continue;

        lld_dump_dr(i, j, d);
      }
    }
  }
}

void lld_print_dr_stats(void) {
  int i, j;
  int depth;
  bf_dma_dr_id_t d;
  uint64_t bytes_processed = 0;
  bf_dma_dr_id_t dr_max = 0;
  lld_dump_drs();

  fprintf(
      reg_get_outstream(),
      "\n-+-+--+-----------+-----------+-----------+-----------+----+------\n");
  fprintf(
      reg_get_outstream(),
      "#|#|dr|desc pushed|byte pushed|desc pulled|byte pulled| occ| name\n");
  fprintf(
      reg_get_outstream(),
      "-+-+--+-----------+-----------+-----------+-----------+----+------\n");
  for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
    if (!lld_dev_ready(i, 0)) continue;
    dr_max = lld_dr_get_max_dr(i, 0);

    for (j = 0; j < BF_MAX_SUBDEV_COUNT; j++) {
      for (d = 0; d <= dr_max; d++) {
        uint32_t dru_base = lld_dr_base_get(i, d);
        lld_dr_view_t *view = &lld_ctx->asic[i][j].dr_view[d];

        /* do not access the DRs that are not implemented on this chip */
        if (dru_base == 0) {
          continue;
        }
        /* do not access the DRs that are not managed by this process */
        if (lld_dr_cfg_depth_get(i, j, d, &depth) != LLD_OK) continue;
        if (depth == 0) continue;

        dr_update_view(view);

        bytes_processed += view->n_bytes;

        fprintf(reg_get_outstream(),
                "%d|%d|%2d| %9" PRIu64 " | %9" PRIu64 " | %9" PRIu64
                " | %9" PRIu64 " |%4d| %s\n",
                i,
                j,
                d,
                view->n_descs,
                view->n_bytes,
                view->n_descs,
                view->n_bytes,
                dr_used(view),
                dr_e_to_str(d));
      }
      fprintf(
          reg_get_outstream(),
          "-+--+-----------+-----------+-----------+-----------+----+------\n");
      fprintf(reg_get_outstream(),
              "Total bytes processed: %" PRIu64 " MB\n",
              bytes_processed / (1 * 1024 * 1024));
    }
  }
}

static void lld_print_one_dr_entry(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   bf_dma_dr_id_t dr_id,
                                   uint64_t *entry) {
  if (!lld_dev_ready(dev_id, subdev_id)) return;
  if (lld_validate_dr_id(dev_id, dr_id)) return;
  lld_dev_t *dev_p = lld_map_subdev_id_to_dev_p(dev_id, subdev_id);
  if (dev_p == NULL) return;
  bool is_tof = bf_is_dev_type_family_tofino(lld_dev_type_get(dev_id));
  bool is_tof2 = bf_is_dev_type_family_tofino2(lld_dev_type_get(dev_id));
  bool is_tof3 = bf_is_dev_type_family_tofino3(lld_dev_type_get(dev_id));





  int depth = 0;
  if (lld_dr_cfg_depth_get(dev_id, subdev_id, dr_id, &depth) != LLD_OK) return;
  if (depth == 0) return;

  switch (dr_id) {
    case lld_dr_fm_pkt_0:
    case lld_dr_fm_pkt_1:
    case lld_dr_fm_pkt_2:
    case lld_dr_fm_pkt_3:
    case lld_dr_fm_pkt_4:
    case lld_dr_fm_pkt_5:
    case lld_dr_fm_pkt_6:
    case lld_dr_fm_pkt_7:
    case lld_dr_fm_lrt:
    case lld_dr_fm_idle:
    case lld_dr_fm_learn:
    case lld_dr_fm_diag:
      fprintf(reg_get_outstream(),
              "Sz %d DMA Addr 0x%" PRIx64,
              256 << (*entry & 0xf),
              (*entry >> 8));
      break;
    case lld_dr_tx_pipe_inst_list_0:
    case lld_dr_tx_pipe_inst_list_1:
    case lld_dr_tx_pipe_inst_list_2:
    case lld_dr_tx_pipe_inst_list_3:
      if (is_tof) {
        fprintf(reg_get_outstream(),
                "%s Sz %d Len %" PRIu64 " Src %" PRIx64 " Dst %" PRIx64
                " MsgId 0x%" PRIx64,
                (*entry & (1ull << 5)) ? "Rd" : "Wr",
                4 << ((*entry >> 6) & 7),
                *entry >> 32,
                *(entry + 1),
                *(entry + 2),
                *(entry + 3));
      } else if (is_tof2 || is_tof3) {
        fprintf(reg_get_outstream(),
                "%s Sz %d MC=%s Vec 0x%" PRIx64 " S+F=%s Len %" PRIu64
                " Src %" PRIx64 " Dst %" PRIx64 " MsgId 0x%" PRIx64,
                (*entry & (1ull << 5)) ? "Rd" : "Wr",
                4 << ((*entry >> 6) & 7),
                ((*entry >> 9) & 1) ? "Y" : "N",
                (*entry >> 10) & 0xF,
                ((*entry >> 14) & 1) ? "Y" : "N",
                *entry >> 32,
                *(entry + 1),
                *(entry + 2),
                *(entry + 3));
      }
      break;
    case lld_dr_tx_pipe_write_block:
      if (is_tof) {
        fprintf(reg_get_outstream(),
                "EntrySz %2d AddrInc %2d MCastEn %d MCastVec %" PRIx64
                " Count %5" PRIu64 " Src %" PRIx64 " Dst %" PRIx64
                " MsgId 0x%" PRIx64,
                4 << ((*entry >> 5) & 7),
                *entry & (1ull << 8) ? 1 : 4,
                *entry & (1ull << 9) ? 1 : 0,
                (*entry >> 10) & INT64_C(0xF),
                (*entry >> 32) / (INT64_C(4) << ((*entry >> 5) & 7)),
                *(entry + 1),
                *(entry + 2),
                *(entry + 3));
      } else if (is_tof2 || is_tof3) {
        fprintf(reg_get_outstream(),
                "EntrySz %2d %sAddrInc %2d MCastEn %d MCastVec %" PRIx64
                " Count %5" PRIu64 " Src %" PRIx64 " Dst %" PRIx64
                " MsgId 0x%" PRIx64,
                4 << ((*entry >> 5) & 7),
                (*entry >> 8) & 1 ? "SingleData " : "",
                1 << ((*entry >> 14) & 7),
                *entry & (1ull << 9) ? 1 : 0,
                (*entry >> 10) & INT64_C(0xF),
                (*entry >> 32) / (INT64_C(4) << ((*entry >> 5) & 7)),
                *(entry + 1),
                *(entry + 2),
                *(entry + 3));
      }
      break;
    case lld_dr_tx_pipe_read_block:
      if (is_tof) {
        fprintf(reg_get_outstream(),
                "EntrySz %2d AddrInc %2d Count %5" PRIu64 " Src %" PRIx64
                " Dst %" PRIx64 " MsgId 0x%" PRIx64,
                4 << ((*entry >> 5) & 7),
                *entry & (1ull << 8) ? 1 : 4,
                (*entry >> 32),
                *(entry + 1),
                *(entry + 2),
                *(entry + 3));
      } else if (is_tof2 || is_tof3) {
        fprintf(reg_get_outstream(),
                "EntrySz %2d AddrInc %2d Count %5" PRIu64 " Src %" PRIx64
                " Dst %" PRIx64 " MsgId 0x%" PRIx64,
                4 << ((*entry >> 5) & 7),
                1 << ((*entry >> 8) & 7),
                (*entry >> 32),
                *(entry + 1),
                *(entry + 2),
                *(entry + 3));
      }
      break;
    case lld_dr_tx_que_write_list:
    case lld_dr_tx_que_write_list_1:
      fprintf(reg_get_outstream(),
              "EntrySz %2d Count %5" PRIu64 " Src %" PRIx64 " MsgId 0x%" PRIx64,
              4 << ((*entry >> 5) & 7),
              (*entry >> 32) / (8 + (4 << ((*entry >> 5) & 7))),
              *(entry + 1),
              *(entry + 3));
      break;
    case lld_dr_tx_pkt_0:
    case lld_dr_tx_pkt_1:
    case lld_dr_tx_pkt_2:
    case lld_dr_tx_pkt_3:
      fprintf(reg_get_outstream(),
              "Size %" PRIu64 " Src %" PRIx64 " MsgId 0x%" PRIx64,
              *entry >> 32,
              *(entry + 1),
              *(entry + 3));
      break;
    case lld_dr_tx_mac_stat:
    case lld_dr_cmp_pipe_inst_list_0:
    case lld_dr_cmp_pipe_inst_list_1:
    case lld_dr_cmp_pipe_inst_list_2:
    case lld_dr_cmp_pipe_inst_list_3:
    case lld_dr_cmp_que_write_list:
    case lld_dr_cmp_pipe_write_blk:
    case lld_dr_cmp_pipe_read_blk:
    case lld_dr_cmp_mac_stat:
    case lld_dr_cmp_tx_pkt_0:
    case lld_dr_cmp_tx_pkt_1:
    case lld_dr_cmp_tx_pkt_2:
    case lld_dr_cmp_tx_pkt_3:
      fprintf(reg_get_outstream(),
              "Start %d End %d Status %" PRIu64 " Attributes 0x%" PRIx64
              " Size 0x%" PRIx64 " MsgId 0x%" PRIx64,
              *entry & 1 ? 1 : 0,
              *entry & 2 ? 1 : 0,
              (*entry >> 5) & 3,
              (*entry >> 7) & 0x1FFFFFF,
              *entry >> 32,
              *(entry + 1));
      break;
    case lld_dr_rx_lrt:
    case lld_dr_rx_idle:
      fprintf(reg_get_outstream(),
              "Status %" PRIu64 " Size 0x%" PRIx64 " Dest 0x%" PRIx64,
              (*entry >> 5) & 3,
              *entry >> 32,
              *(entry + 1));
      break;
    case lld_dr_rx_learn: {
      bf_dma_addr_t dma_addr = *(entry + 1);
      bf_sys_dma_pool_handle_t hndl =
          dev_p->dma_info.dma_buff_info[BF_DMA_PIPE_LEARN_NOTIFY]
              .dma_buf_pool_handle;
      void *vaddr = bf_mem_dma2virt(hndl, dma_addr);
      fprintf(reg_get_outstream(),
              "Start %d End %d Status %" PRIu64 " Pipe %" PRIu64
              " Size 0x%" PRIx64 " Dest 0x%" PRIx64 " (%p)",
              *entry & 1 ? 1 : 0,
              *entry & 2 ? 1 : 0,
              (*entry >> 5) & 3,
              (*entry >> 7) & 7,
              *entry >> 32,
              *(entry + 1),
              vaddr);
    } break;
    case lld_dr_rx_pkt_0:
    case lld_dr_rx_pkt_1:
    case lld_dr_rx_pkt_2:
    case lld_dr_rx_pkt_3:
    case lld_dr_rx_pkt_4:
    case lld_dr_rx_pkt_5:
    case lld_dr_rx_pkt_6:
    case lld_dr_rx_pkt_7:
      fprintf(reg_get_outstream(),
              "Start %d End %d Status %" PRIu64 " Error %" PRIu64
              " Size 0x%" PRIx64 " Dest 0x%" PRIx64,
              *entry & 1 ? 1 : 0,
              *entry & 2 ? 1 : 0,
              (*entry >> 5) & 3,
              (*entry >> 7) & 1,
              *entry >> 32,
              *(entry + 1));
      break;
    case lld_dr_rx_diag:
      fprintf(reg_get_outstream(),
              "Start %d End %d Status %" PRIu64 " Size 0x%" PRIx64
              " Dest 0x%" PRIx64,
              *entry & 1 ? 1 : 0,
              *entry & 2 ? 1 : 0,
              (*entry >> 5) & 3,
              *entry >> 32,
              *(entry + 1));
      break;
    default:
      fprintf(reg_get_outstream(), "Unhandled DR %d", dr_id);
      break;
  }
}

void lld_print_dr_contents(bf_dev_id_t dev_id,
                           bf_subdev_id_t subdev_id,
                           bf_dma_dr_id_t dr_id) {
  if (!lld_dev_ready(dev_id, 0)) return;
  if (lld_validate_dr_id(dev_id, dr_id)) return;
  int depth = 0;
  if (lld_dr_cfg_depth_get(dev_id, subdev_id, dr_id, &depth) != LLD_OK) return;
  if (depth == 0) return;

  lld_dr_view_t *view = &lld_ctx->asic[dev_id][subdev_id].dr_view[dr_id];
  dr_update_view(view);
  for (int i = 0; i < view->n_entries; ++i) {
    int j = (view->head + i) % view->n_entries;
    uint64_t *p = (void *)(uintptr_t)view->base;
    p += view->n_words_per_desc * j;
    fprintf(reg_get_outstream(), "%04d|", j);
    lld_print_one_dr_entry(dev_id, subdev_id, dr_id, p);
    fprintf(reg_get_outstream(), "\n");
  }
}

void lld_log_settings(void) {
  int i, j;
  int dr_max;
  printf("glbl_logs_en: %d\n", lld_log_cfg.glbl_logs_en);
  printf("chip_logs_en: ");
  for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
    if (lld_log_cfg.chip_logs_en[i]) {
      printf("%d:%d ", i, lld_log_cfg.chip_logs_en[i]);
    }
  }
  printf("\n");

  printf("mac_logs_en : ");
  for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
    uint32_t num_subdev = 0;
    lld_sku_get_num_subdev(i, &num_subdev, NULL);
    for (j = 0;
         j < (int)(BF_SUBDEV_PIPE_COUNT * num_subdev * BF_PIPE_PORT_COUNT);
         j++) {
      if (lld_log_cfg.mac_logs_en[i][j]) {
        printf("%d[%d]:%d ", i, j, lld_log_cfg.mac_logs_en[i][j]);
      }
    }
  }
  printf("\n");

  printf("port_logs_en: ");
  for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
    uint32_t num_subdev = 0;
    lld_sku_get_num_subdev(i, &num_subdev, NULL);
    for (j = 0;
         j < (int)(BF_SUBDEV_PIPE_COUNT * num_subdev * BF_PIPE_PORT_COUNT);
         j++) {
      if (lld_log_cfg.port_logs_en[i][j]) {
        printf("%d[%d]:%d ", i, j, lld_log_cfg.port_logs_en[i][j]);
      }
    }
  }
  printf("\n");

  printf("dma_logs_en : ");
  for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
    dr_max = lld_dr_get_max_dr(i, 0);
    for (j = 0; j <= dr_max; j++) {
      if (lld_log_cfg.dma_logs_en[i][j]) {
        printf("%d[%d]:%d ", i, j, lld_log_cfg.dma_logs_en[i][j]);
      }
    }
  }
  printf("\n");
}

/*******************************************************************
 * lld_log_worthy
 *
 * Determine whether or not to log an event.
 *
 * - Logs can be disabled entirely by setting,0
 *     lld_log_cfg.glbl_logs_en=0
 *
 * - If logs are not completely disabled then logging may be enabled
 *   on a per-{ object, verbosity } basis.
 * - Each log has a LOG_TYP, based on the object the log describes.
 * - Each object is described by the LOG_TYP and the first two
 *   parameters, p1 and p2, with p1 generally being "chip"
 * - Each LOG_TYP has a per-object verbosity-level, 0-255, with
 *   "0" meaning disabled. To be logged the event must have a
 *   verbosity-level less-than or equal-to the verbosity-level for
 *   the referenced object.
 *******************************************************************/
int lld_log_worthy(lld_log_type_e typ, int p1, int p2, int p3) {
  if (lld_log_cfg.glbl_logs_en == 0) return 0;

  switch (typ) {
    case LOG_TYP_GLBL:
      /* p1=n/a, p2=n/a, p3=verbosity-level */
      return (lld_log_cfg.glbl_logs_en >= p3);
    case LOG_TYP_CHIP:
      /* p1=chip, p2=n/a, p3=verbosity-level */
      return (lld_log_cfg.chip_logs_en[p1] >= p3);
    case LOG_TYP_MAC:
      /* p1=chip, p2=mac_block, p3=verbosity-level */
      return (lld_log_cfg.mac_logs_en[p1][p2] >= p3);
    case LOG_TYP_PORT:
      /* p1=chip, p2=port, p3=verbosity-level */
      return (lld_log_cfg.port_logs_en[p1][p2] >= p3);
    case LOG_TYP_DMA:
      /* p1=chip, p2=dr_e, p3=verbosity-level */
      if (lld_validate_dr_id(p1, p2)) return -1;
      return (lld_log_cfg.dma_logs_en[p1][p2] >= p3);
  }
  return 0;
}

int lld_log_set(lld_log_type_e typ, int p1, int p2, int p3) {
  if (p3 > 255) return -1;
  uint32_t num_subdev = 0;
  lld_sku_get_num_subdev(p1, &num_subdev, NULL);

  switch (typ) {
    case LOG_TYP_GLBL:
      lld_log_cfg.glbl_logs_en = p3;
      break;
    case LOG_TYP_CHIP:
      /* p1=chip, p2=n/a, p3=verbosity-level */
      if (p1 >= BF_MAX_DEV_COUNT) return -1;
      lld_log_cfg.chip_logs_en[p1] = p3;
      break;
    case LOG_TYP_MAC:
      /* p1=chip, p2=port, p3=verbosity-level */
      if ((p1 >= BF_MAX_DEV_COUNT) ||
          (p2 >= (int)(BF_SUBDEV_PIPE_COUNT * num_subdev * BF_PIPE_PORT_COUNT)))
        return -1;
      lld_log_cfg.mac_logs_en[p1][p2] = p3;
      break;
    case LOG_TYP_PORT:
      /* p1=chip, p2=port, p3=verbosity-level */
      if ((p1 >= BF_MAX_DEV_COUNT) ||
          (p2 >= (int)(BF_SUBDEV_PIPE_COUNT * num_subdev * BF_PIPE_PORT_COUNT)))
        return -1;
      lld_log_cfg.port_logs_en[p1][p2] = p3;
      break;
    case LOG_TYP_DMA:
      /* p1=chip, p2=dr_e, p3=verbosity-level */
      if ((p1 >= BF_MAX_DEV_COUNT) || (p2 >= BF_DMA_MAX_DR)) return -1;
      if (lld_validate_dr_id(p1, p2)) return -1;
      lld_log_cfg.dma_logs_en[p1][p2] = p3;
      break;
  }
  return 0;
}
