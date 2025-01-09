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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <bf_types/bf_types.h>
#include <tofino_regs/tofino.h>
#include <tof2_regs/tof2_reg_drv.h>
#include <tof3_regs/tof3_reg_drv.h>
#include <lld/lld_reg_if.h>
#include "lld.h"
#include "lld_dev.h"
#include "lld_log.h"
#include "lld_csr2jtag.h"

#define csr2jtag_log(fmt, ...)               \
  {                                          \
    struct timeval T;                        \
    gettimeofday(&T, NULL);                  \
    struct tm *LT = localtime(&T.tv_sec);    \
    char B[512] = {0};                       \
    if (LT != NULL) {                        \
      strftime(B, sizeof B, "%a %b %c", LT); \
      fprintf(log,                           \
              "%s.%06ld Line %5d " fmt "\n", \
              B,                             \
              (unsigned long)T.tv_usec,      \
              line_num,                      \
              __VA_ARGS__);                  \
      fflush(log);                           \
    }                                        \
  }

static bf_status_t execute_csr2jtag_script(bf_dev_id_t dev_id,
                                           bf_subdev_id_t subdev_id,
                                           FILE *f,
                                           FILE *log) {
  bf_status_t rc = BF_SUCCESS;
  /* Get the address of various TCU registers for use later. */
  uint32_t ctrl0_addr = 0;
  uint32_t ctrl1_addr = 0;
  uint32_t wrack_addr = 0;
  uint32_t status_addr = 0;
  if (lld_dev_is_tofino(dev_id)) {
    if (subdev_id != 0) {
      return BF_SUCCESS;
    }
    ctrl0_addr = offsetof(Tofino, device_select.misc_regs.tcu_control0);
    ctrl1_addr = offsetof(Tofino, device_select.misc_regs.tcu_control1);
    wrack_addr = offsetof(Tofino, device_select.misc_regs.tcu_wrack);
    status_addr = offsetof(Tofino, device_select.misc_regs.tcu_status);
  } else if (lld_dev_is_tof2(dev_id)) {
    if (subdev_id != 0) {
      return BF_SUCCESS;
    }
    ctrl0_addr = offsetof(tof2_reg, device_select.misc_regs.tcu_control0);
    ctrl1_addr = offsetof(tof2_reg, device_select.misc_regs.tcu_control1);
    wrack_addr = offsetof(tof2_reg, device_select.misc_regs.tcu_wrack);
    status_addr = offsetof(tof2_reg, device_select.misc_regs.tcu_status);
  } else if (lld_dev_is_tof3(dev_id)) {
    ctrl0_addr =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.tcu_control0);
    ctrl1_addr =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.tcu_control1);
    wrack_addr =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.tcu_wrack);
    status_addr =
        offsetof(tof3_reg, device_select.misc_all_regs.misc_regs.tcu_status);
  }

  /* Read the file line-by-line.  Some lines can be long so size the read buffer
   * for the worst case based on file size. */
  struct stat stat_b;
  fstat(fileno(f), &stat_b);
  size_t max_line_len = stat_b.st_size + 1;
  char *l = bf_sys_calloc(max_line_len, 1);
  if (!l) return BF_NO_SYS_RESOURCES;
  int line_num = 0;
  uint32_t last_rd_addr = 0;
  uint32_t last_rd_val = 0;
  char *line = NULL, *x = NULL;
  while (fgets(l, max_line_len, f)) {
    ++line_num;
    line = l;

    /* Remove trailing whitespace. */
    for (int line_end = strlen(l) - 1; line_end > 0 && isspace(l[line_end]);
         --line_end)
      l[line_end] = '\0';

    /* Trim leading white space. */
    while (isspace(*line)) ++line;

    /* Skip empty lines. */
    if (!strlen(line)) continue;

    /* Copy the line since strtok will modify it and we want the original
     * preserved incase we log it. */
    x = bf_sys_strdup(line);

    if (*line == '#') {
      /* Handle comment lines, a few of them have special instructions. */
      if (strstr(line, "DELAY")) {
        /* Parse "# DELAY 123 TCK cycles - 1 cycle = 100ns" */
        char *t = strtok(x, "# \tDELAY"); /* Skip over the leading "# DELAY" */
        if (!t) {
          lld_log("FAILED to parse line %d", line_num);
          rc = BF_UNEXPECTED;
          continue;
        }
        /* Get the cycle count. */
        errno = ENOENT;
        uint32_t cycles = strtoul(t, NULL, 0);
        if ((errno != ENOENT) || (cycles == 0) || (cycles > (UINT_MAX - 9))) {
          perror("Invalid cycles input, set cycle to 1");
          cycles = 1;
        }
        uint32_t microseconds = (cycles + 9) / 10;
        csr2jtag_log("wait %d microseconds", microseconds);
        bf_sys_usleep(microseconds);
      } else if (strstr(line, "Expected Data Packet")) {
        char *t = strtok(x, "b");
        t = strtok(NULL, "b");
        uint32_t mask = 0;
        uint32_t expected = 0;
        for (int i = 0; i < 32 && t && t[i] != '\0'; ++i) {
          if (t[i] == '1') {
            expected = 1 | (expected << 1);
            mask = 1 | (mask << 1);
          } else if (t[i] == '0') {
            expected = expected << 1;
            mask = 1 | (mask << 1);
          } else if (t[i] == 'X') {
            expected = expected << 1;
            mask = mask << 1;
          }
        }
        csr2jtag_log(
            "Expected %08x Mask %08x data %08x", expected, mask, last_rd_val);
        if ((last_rd_val & mask) != (expected & mask)) {
          lld_log(
              "FAILED line %d, read addr 0x%08x expected 0x%08x mask 0x%08x "
              "got 0x%08x",
              line_num,
              last_rd_addr,
              expected,
              mask,
              last_rd_val);
          csr2jtag_log("FAILURE %x != %x", expected & mask, last_rd_val & mask);
          rc = BF_HW_UPDATE_FAILED;
          /*break; Removed the break to continue on error. */
        }
        bf_sys_usleep(100); /* Delay before TCU reads. */
        lld_subdev_read_register(dev_id, subdev_id, last_rd_addr, &last_rd_val);
        csr2jtag_log("Read addr %08x got %08x", last_rd_addr, last_rd_val);
      }
    } else {
      char *t = strtok(x, " \t");
      if (!t) {
        lld_log("Unexpected line (rd/wr): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }
      /* Non-comment lines must be either a read or a write. */
      bool rd = 0 == strcmp(t, "rr");
      bool wr = 0 == strcmp(t, "wr");
      if (!rd && !wr) {
        lld_log("Unexpected line (rd/wr): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }

      /* Must be to dev_0 device_select misc_regs */
      t = strtok(NULL, " \t");
      if (!t || strcmp(t, "dev_0")) {
        lld_log("Unexpected line (dev): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }
      t = strtok(NULL, " \t");
      if (!t || strcmp(t, "device_select")) {
        lld_log("Unexpected line (dev-sel): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }
      t = strtok(NULL, " \t");
      if (!t || strcmp(t, "misc_regs")) {
        lld_log("Unexpected line (misc): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }

      /* Can access tcu_control0, tcu_control1, tcu_wrack or tcu_status */
      t = strtok(NULL, " \t");
      if (!t) {
        lld_log("Unexpected line (reg): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }

      bool ctrl0 = 0 == strcmp(t, "tcu_control0");
      bool ctrl1 = 0 == strcmp(t, "tcu_control1");
      bool wrack = 0 == strcmp(t, "tcu_wrack");
      bool status = 0 == strcmp(t, "tcu_status");
      if (!(ctrl0 || ctrl1 || wrack || status)) {
        lld_log("Unexpected line (reg): line %d \"%s\"", line_num, l);
        rc = BF_UNEXPECTED;
        break;
      }

      /* Writes come with a value. */
      uint32_t val = 0;
      if (wr) {
        t = strtok(NULL, " \t");
        if (!t) {
          lld_log("Unexpected line (wr-val): line %d \"%s\"", line_num, l);
          rc = BF_UNEXPECTED;
          break;
        }
        val = strtoul(t, NULL, 16);
      }

      /* Execute the read or write. */
      uint32_t addr = 0;
      if (ctrl0) {
        addr = ctrl0_addr;
      } else if (ctrl1) {
        addr = ctrl1_addr;
      } else if (wrack) {
        addr = wrack_addr;
      } else if (status) {
        addr = status_addr;
      }
      if (wr) {
        csr2jtag_log("Writing %08x to address %08x", val, addr);
        lld_subdev_write_register(dev_id, subdev_id, addr, val);
        bf_sys_usleep(100); /* Delay after each TCU access. */
      } else {
        bf_sys_usleep(100); /* Delay before TCU reads. */
        lld_subdev_read_register(dev_id, subdev_id, addr, &val);
        csr2jtag_log("Read %08x from address %08x", val, addr);
        last_rd_addr = addr;
        last_rd_val = val;
      }
    }
    bf_sys_free(x);
    x = NULL;
  }
  if (x) bf_sys_free(x);
  bf_sys_free(l);
  return rc;
}

static bf_status_t run_csr2jtag_from_file(bf_dev_id_t dev_id,
                                          bf_subdev_id_t subdev_id,
                                          const char *filename) {
  bf_status_t sts = BF_SUCCESS;
  FILE *f = NULL, *log = NULL;
  char *log_filename = bf_sys_calloc(strlen(filename) + 5, 1);
  if (!log_filename) {
    sts = BF_NO_SYS_RESOURCES;
    goto done;
  }
  sprintf(log_filename, "%s.log", filename);

  f = fopen(filename, "r");
  if (!f) {
    lld_log("Could not open \"%s\" for reading", filename);
    sts = BF_INVALID_ARG;
    goto done;
  }
  log = fopen(log_filename, "w");
  if (!log) {
    lld_log("Could not open \"%s\" for writing", log_filename);
    sts = BF_INVALID_ARG;
    goto done;
  }

  lld_log("Executing %s", filename);
  sts = execute_csr2jtag_script(dev_id, subdev_id, f, log);
  lld_log("Done with %s, status %s", filename, bf_err_str(sts));
done:
  if (f) fclose(f);
  if (log) fclose(log);
  if (log_filename) bf_sys_free(log_filename);
  return sts;
}

static bf_status_t run_suite(bf_dev_id_t dev_id,
                             bf_subdev_id_t subdev_id,
                             const char *fname) {
  int test_list_size = 8192;
  int num_tests = 0;
  FILE *f = NULL;
  char **test_list = NULL;
  if ((f = fopen(fname, "r")) != NULL) {
    char l[2048] = {0};
    test_list = bf_sys_calloc(sizeof *test_list, test_list_size);
    if (!test_list) {
      fclose(f);
      return BF_NO_SYS_RESOURCES;
    }
    while (fgets(l, sizeof l, f)) {
      /* Remove trailing whitespace. */
      for (int line_end = strlen(l) - 1; line_end > 0 && isspace(l[line_end]);
           --line_end)
        l[line_end] = '\0';
      /* Remove leading whitespace. */
      char *line = l;
      while (isspace(*line)) ++line;
      if (strlen(line)) {
        lld_log("Test group %s: Found test \"%s\"", fname, line);
        test_list[num_tests++] = bf_sys_strdup(line);
        if (num_tests == test_list_size) break;
      }
    }
    fclose(f);
  }

  bf_status_t sts = BF_SUCCESS;
  bf_status_t rc = sts;
  for (int i = 0; i < num_tests; ++i) {
    sts = run_csr2jtag_from_file(dev_id, subdev_id, test_list[i]);
    if (sts != BF_SUCCESS) {
      lld_log("%s \"%s\" failed (%s)", fname, test_list[i], bf_err_str(sts));
      if (rc == BF_SUCCESS) rc = sts;
    }
    bf_sys_free(test_list[i]);
  }
  bf_sys_free(test_list);

  return rc;
}

bf_status_t lld_csr2jtag_run_pre_efuse(bf_dev_id_t dev_id,
                                       bf_subdev_id_t subdev_id) {
  return run_suite(dev_id, subdev_id, "/csr2jtag_pre_efuse_test_list");
}

bf_status_t lld_csr2jtag_run_post_efuse(bf_dev_id_t dev_id,
                                        bf_subdev_id_t subdev_id) {
  return run_suite(dev_id, subdev_id, "/csr2jtag_post_reset_test_list");
}

bf_status_t lld_csr2jtag_run_one_file(bf_dev_id_t dev_id,
                                      bf_subdev_id_t subdev_id,
                                      const char *fname) {
  return run_csr2jtag_from_file(dev_id, subdev_id, fname);
}

bf_status_t lld_csr2jtag_run_suite(bf_dev_id_t dev_id,
                                   bf_subdev_id_t subdev_id,
                                   const char *fname) {
  return run_suite(dev_id, subdev_id, fname);
}
