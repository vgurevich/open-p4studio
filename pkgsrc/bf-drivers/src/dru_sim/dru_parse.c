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

#include <assert.h>
#include <dru_sim/dru_sim.h>
#include <inttypes.h>
#include <lld/lld_inst_list_fmt.h>

#include <tofino_regs/tofino.h>
#include <bf_types/bf_types.h>

int g_prsr_cfg_debug_mode = 0;
int g_debug_mode_parse = 0;

#ifdef TARGET_IS_MODEL
extern void simulator_pcie_pkt_in(dru_dev_id_t asic, uint8_t *buf, int len);
extern void simulator_reg_write(dru_dev_id_t asic,
                                unsigned int addr,
                                unsigned int value);
extern unsigned int simulator_reg_read(dru_dev_id_t asic, unsigned int addr);
extern void simulator_ind_write(dru_dev_id_t asic,
                                uint64_t address,
                                uint64_t data0,
                                uint64_t data1);
extern void simulator_ind_read(dru_dev_id_t asic,
                               uint64_t address,
                               uint64_t *data0,
                               uint64_t *data1);
extern void model_tcam_copy_word(dru_dev_id_t asic,
                                 int pipe,
                                 int stage,
                                 int src_table_id,
                                 int dst_table_id,
                                 int num_tables,
                                 int num_words,
                                 int adr_incr_dir,
                                 uint32_t src_address,
                                 uint32_t dst_address);
extern void model_set_tcam_writereg(dru_dev_id_t asic,
                                    int pipe,
                                    int stage,
                                    int mem,
                                    uint32_t address,
                                    uint64_t data_0,
                                    uint64_t data_1,
                                    bool write_tcam);
extern void simulator_incr_time(uint64_t amount);
extern bf_dev_family_t lld_dev_family_get(bf_dev_id_t dev_id);
extern bool lld_dev_is_tofino(bf_dev_id_t dev_id);
extern bool lld_dev_is_tof2(bf_dev_id_t dev_id);
extern bool lld_dev_is_tof3(bf_dev_id_t dev_id);

void dru_sim_update_il_addr_inst(dru_dev_id_t asic, int which, uint64_t val);
uint64_t dru_sim_get_il_addr_inst(dru_dev_id_t asic, int which);
#endif  // TARGET_IS_MODEL
/*************************************************************************
 * get_pipes_per_die
 *************************************************************************/
static inline int get_pipes_per_die(bf_dev_family_t dev_family) {
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:
      return 4;




    default:
      return 0;
  }
}




































































/*************************************************************************
 * dru_parse_inst_list
 *************************************************************************/
void dru_parse_inst_list(dru_dev_id_t asic,
                         int dru,
                         bool is_mcast,
                         int mcast_vec,
                         uint32_t *list,
                         int len) {
#ifdef TARGET_IS_MODEL
  pipe_instr_common_wd0_t *hdr;
  pipe_instr_write_reg_t *reg_wr;
  uint64_t data_bus_0 = 0;
  uint64_t data_bus_1 = 0;
  uint64_t addr_bus;
  int n;
  unsigned int reg_addr, reg_val;

  if (g_debug_mode_parse) {
    printf("dru_parse_inst_list: asic=%d, dru=%d, len=%d\n", asic, dru, len);
  }

  if (dru >= 4) return;        // invalid DRU identified
  if ((len & 3) != 0) return;  // invalid length identified
  bf_dev_family_t dev_family = lld_dev_family_get(asic);





  for (n = 0; n < len / 4;) {
    bool handled = false;
    hdr =
        (pipe_instr_common_wd0_t *)&list[n];  // step over previous instruction
    switch (hdr->pipe_ring_addr_type) {
      case addr_type_register:
        reg_wr = (pipe_instr_write_reg_t *)hdr;

        switch (dev_family) {
          case BF_DEV_FAMILY_TOFINO:
            reg_addr = tofino_pipes_address |
                       //((upper_10_bits[asic][dru] & 0x7E) << 18ull) |
                       ((dru_sim_get_il_addr_inst(asic, dru) & 0x7E) << 18ull) |
                       reg_wr->head.tf1.reg_address;
            break;
          case BF_DEV_FAMILY_TOFINO2:
          case BF_DEV_FAMILY_TOFINO3:
            reg_addr =
                ((dru_sim_get_il_addr_inst(asic, dru) & 0xFFC) << 17ull) |
                (reg_wr->head.tf1.reg_address & 0x7FFFF);
            break;
          default:
            return;
        }
        reg_val = reg_wr->data;
        if (is_mcast) {
          for (int pipe = 0; pipe < 4; ++pipe) {
            uint32_t a;
            if (!(mcast_vec & (1 << pipe))) continue;
            if (dev_family == BF_DEV_FAMILY_TOFINO2 ||
                dev_family == BF_DEV_FAMILY_TOFINO3) {
              a = (reg_addr & ~(3 << 24)) | (pipe << 24);
            } else {
              return;
            }
            simulator_reg_write(asic, a, reg_val);
          }
        } else {
          simulator_reg_write(asic, reg_addr, reg_val);
        }
        n += 2;
        if (g_debug_mode_parse) {
          printf("::: write_reg::: %d<%d>: \n", asic, dru);
          printf(":::::: addr: %08x\n", reg_addr);
          printf(":::::: data: %08x\n", reg_val);
        }
        handled = true;
        break;
      case addr_type_instruction:
        switch (hdr->specific) {
          case INSTR_OPCODE_SELECT_DEST: {
            // upper_10_bits[asic][dru] = *(((uint32_t *)hdr) + 1);
            dru_sim_update_il_addr_inst(asic, dru, *(((uint32_t *)hdr) + 1));
            n += 2;

            if (g_debug_mode_parse) {
              printf("::: dest_select:: %d<%d>: upper 10b=%03" PRIx64 "\n",
                     asic,
                     dru,
                     // upper_10_bits[asic][dru]);
                     dru_sim_get_il_addr_inst(asic, dru));
            }
          }
            handled = true;
            break;
          case INSTR_OPCODE_SELECT_DEST_STAGE: {
            // upper_10_bits[asic][dru] = (upper_10_bits[asic][dru] & ~0x1F) |
            //                           (*(((uint32_t *)hdr) + 1) & 0x1F);
            uint64_t cur = dru_sim_get_il_addr_inst(asic, dru);
            switch (lld_dev_family_get(asic)) {
              case BF_DEV_FAMILY_TOFINO:
                dru_sim_update_il_addr_inst(
                    asic,
                    dru,
                    (cur & ~0x1F) | (*(((uint32_t *)hdr) + 1) & 0x1F));
                break;
              case BF_DEV_FAMILY_TOFINO2:
              case BF_DEV_FAMILY_TOFINO3:
                dru_sim_update_il_addr_inst(
                    asic,
                    dru,
                    (cur & ~0x7F) | (*(((uint32_t *)hdr) + 1) & 0x7F));
                break;
              default:
                return;
            }
            n += 2;

            if (g_debug_mode_parse) {
              printf("::: dest_select_stage:: %d<%d>: upper 10b=%03" PRIx64
                     "\n",
                     asic,
                     dru,
                     // upper_10_bits[asic][dru]);
                     dru_sim_get_il_addr_inst(asic, dru));
            }
          }
            handled = true;
            break;
          default:
            handled = false;
            break;
        }
        break;
      default:
        handled = false;
        break;
    }

    if (!handled) {
      uint32_t *addr = (uint32_t *)hdr;
      // addr_bus = (upper_10_bits[asic][dru] << 32ull) | *addr;
      addr_bus = (dru_sim_get_il_addr_inst(asic, dru) << 32ull) | *addr;
      /* For ilist entries of not type addr_type_instruction clear bits 29:28
       */
      if (hdr->pipe_ring_addr_type != addr_type_instruction) {
        addr_bus &= (~(0x3ull << 28ull));
      }
      addr++;

      data_bus_0 = 0;
      data_bus_1 = 0;
      switch (hdr->data_width) {
        case 0:
          data_bus_0 = 0;
          data_bus_1 = 0;
          n += 1;
          break;
        case 1:
          memcpy(&data_bus_0, addr, 4);
          data_bus_1 = 0;
          n += 2;
          break;
        case 2:
          memcpy(&data_bus_0, addr, 8);
          data_bus_1 = 0;
          n += 3;
          break;
        case 3:
          memcpy(&data_bus_0, addr, 8);
          memcpy(&data_bus_1, addr + 2, 8);
          n += 5;
          break;
        default:
          assert(0);
          break;
      }
      if (is_mcast) {
        for (int pipe = 0; pipe < 4; ++pipe) {
          if (!(mcast_vec & (1 << pipe))) continue;
          uint64_t a = (addr_bus & ~(3ull << 39)) | ((uint64_t)pipe << 39);
          simulator_ind_write(asic, a, data_bus_0, data_bus_1);
        }
      } else {
        simulator_ind_write(asic, addr_bus, data_bus_0, data_bus_1);
      }

      if (g_debug_mode_parse) {
        printf("::: inst::: %d<%d>: \n", asic, dru);
        printf(":::::: addr: %08" PRIx64 "\n", addr_bus);
        printf(":::::: data: %08" PRIx64 " %08" PRIx64 "\n",
               data_bus_1,
               data_bus_0);
      }
    }
  }
#else
  (void)asic;
  (void)dru;
  (void)is_mcast;
  (void)mcast_vec;
  (void)list;
  (void)len;
#endif  // TARGET_IS_MODEL
}

/*************************************************************************
 * dru_parse_inst_list_read
 *************************************************************************/
void dru_parse_inst_list_read(dru_dev_id_t asic,
                              int dru,
                              uint32_t *list,
                              int len,
                              int entry_sz,
                              uint8_t *wr_buffer,
                              int *wr_buf_sz) {
#ifdef TARGET_IS_MODEL
  pipe_instr_common_wd0_t *hdr;
  pipe_instr_read_reg_t *reg_rd;
  uint64_t data_bus_0 = 0;
  uint64_t data_bus_1 = 0;
  uint64_t addr_bus;
  int n, num_responses = 0;
  unsigned int reg_addr, reg_val;
  uint32_t *rsp_32b = (uint32_t *)wr_buffer;
  uint64_t *rsp_64b = (uint64_t *)wr_buffer;
  uint64_t *rsp_128b = (uint64_t *)wr_buffer;

  if (g_debug_mode_parse) {
    printf(
        "dru_parse_inst_list_read: asic=%d, dru=%d, len=%d\n", asic, dru, len);
  }

  if (dru >= 4) return;        // invalid DRU identified
  if ((len & 3) != 0) return;  // invalid length identified

  for (n = 0; n < len / 4;) {
    bool handled = false;
    hdr =
        (pipe_instr_common_wd0_t *)&list[n];  // step over previous instruction
    n += (0 == hdr->data_width)
             ? 1
             : (1 == hdr->data_width) ? 2 : (2 == hdr->data_width) ? 3 : 5;

    switch (hdr->pipe_ring_addr_type) {
      case addr_type_register:
        reg_rd = (pipe_instr_read_reg_t *)hdr;
        switch (lld_dev_family_get(asic)) {
          case BF_DEV_FAMILY_TOFINO:
            reg_addr = tofino_pipes_address |
                       //((upper_10_bits[asic][dru] & 0x7E) << 18ull) |
                       ((dru_sim_get_il_addr_inst(asic, dru) & 0x7E) << 18ull) |
                       reg_rd->reg_address;
            break;
          case BF_DEV_FAMILY_TOFINO2:
          case BF_DEV_FAMILY_TOFINO3:

            // reg_addr = ((upper_10_bits[asic][dru] & 0xFFC) << 17ull) |
            //           (reg_rd->reg_address & 0x7FFFF);
            reg_addr =
                ((dru_sim_get_il_addr_inst(asic, dru) & 0xFFC) << 17ull) |
                (reg_rd->reg_address & 0x7FFFF);
            break;
          default:
            return;
        }
        reg_val = simulator_reg_read(asic, reg_addr);
        // copy to rsp using appropriate width
        switch (entry_sz) {
          case 4:
            *rsp_32b++ = (uint32_t)reg_val;
            break;
          case 8:
            *rsp_64b++ = (uint64_t)reg_val;
            break;
          case 16:
            *rsp_128b++ = (uint64_t)reg_val;
            *rsp_128b++ = 0ull;
            break;
          default:
            printf("Instruction list read: Invalid entry_sz(2) <%d>\n",
                   entry_sz);
            break;
        }
        num_responses++;

        if (g_debug_mode_parse) {
          printf("::: read_reg::: %d<%d>: \n", asic, dru);
          printf(":::::: addr: %08x\n", reg_addr);
          printf(":::::: data: %08x\n", reg_val);
        }
        handled = true;
        break;
      case addr_type_instruction:
        switch (hdr->specific) {
          case INSTR_OPCODE_SELECT_DEST: {
            // upper_10_bits[asic][dru] = *(((uint32_t *)hdr) + 1);
            dru_sim_update_il_addr_inst(asic, dru, *(((uint32_t *)hdr) + 1));

            if (g_debug_mode_parse) {
              printf("::: dest_select:: %d<%d>: upper 10b=%03" PRIx64 "\n",
                     asic,
                     dru,
                     // upper_10_bits[asic][dru]);
                     dru_sim_get_il_addr_inst(asic, dru));
            }
          }
            handled = true;
            break;
          case INSTR_OPCODE_SELECT_DEST_STAGE: {
            // upper_10_bits[asic][dru] = (upper_10_bits[asic][dru] & ~0x1F) |
            //                           (*(((uint32_t *)hdr) + 1) & 0x1F);
            uint64_t cur = dru_sim_get_il_addr_inst(asic, dru);
            switch (lld_dev_family_get(asic)) {
              case BF_DEV_FAMILY_TOFINO:
                dru_sim_update_il_addr_inst(
                    asic,
                    dru,
                    (cur & ~0x1F) | (*(((uint32_t *)hdr) + 1) & 0x1F));
                break;
              case BF_DEV_FAMILY_TOFINO2:
              case BF_DEV_FAMILY_TOFINO3:

                dru_sim_update_il_addr_inst(
                    asic,
                    dru,
                    (cur & ~0x7F) | (*(((uint32_t *)hdr) + 1) & 0x7F));
                break;
              default:
                return;
            }

            if (g_debug_mode_parse) {
              printf("::: dest_select_stage:: %d<%d>: upper 10b=%03" PRIx64
                     "\n",
                     asic,
                     dru,
                     // upper_10_bits[asic][dru]);
                     dru_sim_get_il_addr_inst(asic, dru));
            }
          }
            handled = true;
            break;
          default:
            handled = false;
            break;
        }
        break;
      default:
        handled = false;
        break;
    }

    if (!handled) {
      uint32_t *addr = (uint32_t *)hdr;
      // addr_bus = (upper_10_bits[asic][dru] << 32ull) | *addr;
      addr_bus = (dru_sim_get_il_addr_inst(asic, dru) << 32ull) | *addr;
      /* For ilist entries of not type addr_type_instruction clear bits 29:28 */
      if (hdr->pipe_ring_addr_type != addr_type_instruction) {
        addr_bus &= (~(0x3ull << 28ull));
      }
      addr++;

      simulator_ind_read(asic, addr_bus, &data_bus_0, &data_bus_1);
      // copy to rsp using appropriate width
      switch (entry_sz) {
        case 4:
          *rsp_32b++ = (uint32_t)(data_bus_0 >> 32ull);
          break;
        case 8:
          *rsp_64b++ = (uint64_t)data_bus_0;
          break;
        case 16:
          *rsp_128b++ = (uint64_t)data_bus_0;
          *rsp_128b++ = (uint64_t)data_bus_1;
          break;
        default:
          printf("Instruction list read: Invalid entry_sz(3) <%d>\n", entry_sz);
          break;
      }
      num_responses++;

      if (g_debug_mode_parse) {
        printf("::: inst-rd::: %d<%d>: \n", asic, dru);
        printf(":::::: addr: %08" PRIx64 "\n", addr_bus);
        printf(":::::: data: %08" PRIx64 " %08" PRIx64 "\n",
               data_bus_1,
               data_bus_0);
      }
    }
  }
  *wr_buf_sz = num_responses * entry_sz;

#else
  (void)asic;
  (void)dru;
  (void)list;
  (void)len;
  (void)entry_sz;
  (void)wr_buffer;
  (void)wr_buf_sz;
#endif  // TARGET_IS_MODEL
}

/*************************************************************************
 * dru_parse_write_blk
 *************************************************************************/
#ifdef TARGET_IS_MODEL
void populate_data_bus(const uint32_t *wd_ptr,
                       const int entry_sz,
                       uint64_t *data_bus_0,
                       uint64_t *data_bus_1) {
  /*
   * Copy words from `wd_ptr` into `data_bus_0` and `data_bus_1` according to
   * given size `entry_sz`.
   */
  switch (entry_sz) {
    case 4:
      *data_bus_0 = (uint64_t)(*wd_ptr);
      *data_bus_1 = 0ull;
      break;
    case 8:
      *data_bus_0 = (uint64_t)(*wd_ptr) | ((uint64_t) * (wd_ptr + 1) << 32);
      *data_bus_1 = 0ull;
      break;
    case 16:
      *data_bus_0 =
          (uint64_t) * (wd_ptr + 0) | ((uint64_t) * (wd_ptr + 1) << 32);
      *data_bus_1 =
          (uint64_t) * (wd_ptr + 2) | ((uint64_t) * (wd_ptr + 3) << 32);
      break;
    default:
      assert(0);
  }
}
#endif
void dru_parse_write_blk(dru_dev_id_t asic,
                         uint32_t *wd_ptr,
                         uint64_t dest_addr,
                         int entry_sz,
                         int n_entries,
                         bool single_mode,
                         int addr_step) {
#ifdef TARGET_IS_MODEL
  uint64_t data_bus_0;
  uint64_t data_bus_1;
  uint64_t addr_bus;
  int n;
  addr_bus = dest_addr;
  bf_dev_family_t dev_family = lld_dev_family_get(asic);
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      if (g_prsr_cfg_debug_mode) {
        printf("dru_parse_write_blk: asic=%d, addr=%" PRIx64
               ",  entry_sz=%d, # entries=%d\n",
               asic,
               addr_bus,
               entry_sz,
               n_entries);
      }
      int step = addr_step ? 1 : 4;  // convert addr_type to step (only for tf1)
      for (n = 0; n < n_entries;
           n++, addr_bus += step, wd_ptr += entry_sz / 4) {
        populate_data_bus(wd_ptr, entry_sz, &data_bus_0, &data_bus_1);

        if (1 && g_prsr_cfg_debug_mode) {
          printf("::: WR BLK::: %d: %d of %d\n", asic, n, n_entries);
          printf(":::::: addr: %08" PRIx64 "\n", addr_bus);
          printf(":::::: data: %08" PRIx64 " %08" PRIx64 "\n",
                 data_bus_1,
                 data_bus_0);
        }

        /* For ilist entries of not type addr_type_instruction clear bits 29:28
         */
        addr_bus &= (~(0x3ull << 28ull));

        /* Detect register writes and handle them specially. */
        if ((addr_bus >> 30) & 0x3) {
          simulator_ind_write(asic, addr_bus, data_bus_0, data_bus_1);
        } else {
          /* Extract the pipe, stage, and 19 bit register address from the
           * tofino address. */
          uint64_t p = (addr_bus >> 37) & 0x3;
          uint64_t s = (addr_bus >> 33) & 0xF;
          uint64_t r = addr_bus & 0x7FFFF;
          /* Build a direct PCIe address from the extracted data. */
          uint32_t reg_addr = (1 << 25) | (p << 23) | (s << 19) | r;
          uint32_t reg_data = data_bus_0 & 0xFFFFFFFF;
          simulator_reg_write(asic, reg_addr, reg_data);
        }
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:

      if (g_prsr_cfg_debug_mode) {
        printf("dru_parse_write_blk: asic=%d, addr=%" PRIx64
               ",  entry_sz=%d, # entries=%d\n",
               asic,
               addr_bus,
               entry_sz,
               n_entries);
      }
      bool is_not_reg = (dest_addr >> 30) & 3;
      int wd_ptr_step = !single_mode ? entry_sz / 4 : 0;
      for (n = 0; n < n_entries;
           n++, addr_bus += addr_step, wd_ptr += wd_ptr_step) {
        populate_data_bus(wd_ptr, entry_sz, &data_bus_0, &data_bus_1);

        /* For ilist entries of not type addr_type_instruction clear bits 29:28
         */
        addr_bus &= (~(0x3ull << 28ull));

        /* Detect register writes and handle them specially. */
        if (is_not_reg) {
          simulator_ind_write(asic, addr_bus, data_bus_0, data_bus_1);
        } else {
          /* Extract the pipe, stage, and 19 bit register address from the
           * tofino address. */
          uint64_t p, s, r;
          uint32_t reg_addr;
          if (dev_family == BF_DEV_FAMILY_TOFINO2 ||
              dev_family == BF_DEV_FAMILY_TOFINO3) {
            p = (addr_bus >> 39) & 0x3;
            s = (addr_bus >> 34) & 0x1F;
            r = addr_bus & 0x7FFFF;
            /* Build a direct PCIe address from the extracted data. */
            reg_addr = (1 << 26) | (p << 24) | (s << 19) | r;
          }







          uint32_t reg_data = data_bus_0 & 0xFFFFFFFF;
          simulator_reg_write(asic, reg_addr, reg_data);
        }
      }
      break;
    default:
      break;
  }
#else
  (void)asic;
  (void)wd_ptr;
  (void)dest_addr;
  (void)entry_sz;
  (void)n_entries;
  (void)single_mode;
  (void)addr_step;

#endif  // TARGET_IS_MODEL
}

/*************************************************************************
 * dru_parse_write_blk_mcast
 *************************************************************************/
void dru_parse_write_blk_mcast(dru_dev_id_t asic,
                               uint32_t *wd_ptr,
                               uint64_t dest_addr,
                               int entry_sz,
                               int n_entries,
                               int mcast_vector,
                               bool single_mode,
                               int addr_step) {
#ifdef TARGET_IS_MODEL
  uint64_t data_bus_0;
  uint64_t data_bus_1;
  uint64_t addr_bus;
  uint64_t p, pipe_per_die;
  uint64_t pipe_mask;
  int n;
  uint32_t wd_offset;

  bf_dev_family_t dev_family = lld_dev_family_get(asic);
  pipe_per_die = get_pipes_per_die(dev_family);
  if (pipe_per_die == 0) return;
/* Handle the model-increment-time magic address for all tofino types. */
#define BF_DRU_SIM_TOF_ALL_INCR_TIME_ADDR 0x656d695472636e49

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      pipe_mask = ~(7ull << 37ull);
      int step = addr_step ? 1 : 4;  // convert addr_type to step
      if (g_prsr_cfg_debug_mode) {
        printf("dru_parse_write_blk_mcast: asic=%d, addr=%" PRIx64
               ",  entry_sz=%d, # entries=%d, vec=%x\n",
               asic,
               dest_addr,
               entry_sz,
               n_entries,
               mcast_vector);
      }
      if (dest_addr == BF_DRU_SIM_TOF_ALL_INCR_TIME_ADDR) {
        populate_data_bus(wd_ptr, entry_sz, &data_bus_0, &data_bus_1);
        simulator_incr_time(data_bus_0);
        return;
      }

      for (p = 0; p < pipe_per_die; p++) {
        if ((mcast_vector & (1 << p))) {
          addr_bus = dest_addr;
          wd_offset = 0;
          for (n = 0; n < n_entries;
               n++, addr_bus += step, wd_offset += entry_sz / 4) {
            populate_data_bus(
                wd_ptr + wd_offset, entry_sz, &data_bus_0, &data_bus_1);
            if (1 && g_prsr_cfg_debug_mode) {
              printf("::: WR BLK::: %d: %d of %d\n", asic, n, n_entries);
              printf(":::::: addr: %08" PRIx64 "\n", addr_bus);
              printf(":::::: data: %08" PRIx64 " %08" PRIx64 "\n",
                     data_bus_1,
                     data_bus_0);
            }

            addr_bus = addr_bus & pipe_mask;  // mask out any extant pipe-id

            /* Detect register writes and handle them specially. */
            if ((addr_bus >> 30) & 0x3) {
              simulator_ind_write(
                  asic, addr_bus | (p << 37ull), data_bus_0, data_bus_1);
            } else {
              /* Extract the stage and 19 bit register address from the
               * tofino address. */
              uint64_t s = (addr_bus >> 33) & 0xF;
              uint64_t r = addr_bus & 0x7FFFF;
              /* Build a direct PCIe address from the extracted data. */
              uint32_t reg_addr = (1 << 25) | (p << 23) | (s << 19) | r;
              uint32_t reg_data = data_bus_0 & 0xFFFFFFFF;
              simulator_reg_write(asic, reg_addr, reg_data);
            }
          }
        }
      }
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:

      pipe_mask = ~(3ull << 39ull);
      int wd_ptr_step = !single_mode ? entry_sz / 4 : 0;
      bool is_not_reg = (dest_addr >> 30) & 0x3;

      if (dest_addr == BF_DRU_SIM_TOF_ALL_INCR_TIME_ADDR) {
        populate_data_bus(wd_ptr, entry_sz, &data_bus_0, &data_bus_1);
        simulator_incr_time(data_bus_0);
        return;
      }
      for (p = 0; p < pipe_per_die; p++) {
        if ((mcast_vector & (1 << p))) {
          addr_bus = dest_addr;
          wd_offset = 0;
          for (n = 0; n < n_entries;
               n++, addr_bus += addr_step, wd_offset += wd_ptr_step) {
            populate_data_bus(
                wd_ptr + wd_offset, entry_sz, &data_bus_0, &data_bus_1);

            addr_bus = addr_bus & pipe_mask;  // mask out any extant pipe-id

            /* Detect register writes and handle them specially. */
            if (is_not_reg) {
              simulator_ind_write(
                  asic, addr_bus | (p << 39), data_bus_0, data_bus_1);
            } else {
              uint32_t reg_addr;
              uint64_t s, r;
              if (dev_family == BF_DEV_FAMILY_TOFINO2 ||
                  dev_family == BF_DEV_FAMILY_TOFINO3) {
                /* Extract the stage and 19 bit register address from the
                 * tofino address. */
                s = (addr_bus >> 34) & 0x1F;
                r = addr_bus & 0x7FFFF;
                /* Build a direct PCIe address from the extracted data. */
                reg_addr = (1 << 26) | (p << 24) | (s << 19) | r;






              }
              uint32_t reg_data = data_bus_0 & 0xFFFFFFFF;
              simulator_reg_write(asic, reg_addr, reg_data);
            }
          }
        }
      }
      break;
    default:
      break;
  }
#else
  (void)asic;
  (void)wd_ptr;
  (void)dest_addr;
  (void)entry_sz;
  (void)n_entries;
  (void)mcast_vector;
  (void)single_mode;
  (void)addr_step;

#endif  // TARGET_IS_MODEL
}
/*************************************************************************
 * dru_parse_que_write_list
 *************************************************************************/
void dru_parse_que_write_list(dru_dev_id_t asic,
                              uint32_t *wd_ptr,
                              int entry_sz,
                              int n_entries) {
#ifdef TARGET_IS_MODEL
  uint64_t data_bus_0 = 0;
  uint64_t data_bus_1 = 0;
  uint64_t addr_bus;
  int n;

  if (g_prsr_cfg_debug_mode) {
    printf("dru_parse_que_write_list: asic=%d, entry_sz=%d, # entries=%d\n",
           asic,
           entry_sz,
           n_entries);
  }

  for (n = 0; n < n_entries; ++n) {
    addr_bus = *((uint64_t *)wd_ptr);
    wd_ptr += 2;
    switch (entry_sz) {
      case 4:
        data_bus_0 = *wd_ptr;
        data_bus_1 = UINT64_C(0);
        wd_ptr += 1;
        break;
      case 8:
        data_bus_0 = *(wd_ptr + 1);
        data_bus_0 = (data_bus_0 << 32) | *wd_ptr;
        data_bus_1 = UINT64_C(0);
        wd_ptr += 2;
        break;
      case 16:
        data_bus_0 = *(wd_ptr + 1);
        data_bus_0 = (data_bus_0 << 32) | *wd_ptr;
        data_bus_1 = *(wd_ptr + 3);
        data_bus_1 = (data_bus_1 << 32) | *(wd_ptr + 2);
        wd_ptr += 4;
        break;
      default:
        assert(0);
    }

    if (1 && g_prsr_cfg_debug_mode) {
      printf("::: Q WRL::: %d: %d of %d\n", asic, n + 1, n_entries);
      printf(":::::: addr: %08" PRIx64 "\n", addr_bus);
      printf(
          ":::::: data: %08" PRIx64 " %08" PRIx64 "\n", data_bus_1, data_bus_0);
    }

    simulator_ind_write(asic, addr_bus, data_bus_0, data_bus_1);
  }
#else
  (void)asic;
  (void)wd_ptr;
  (void)entry_sz;
  (void)n_entries;

#endif  // TARGET_IS_MODEL
}

/*************************************************************************
 * dru_parse_read_blk
 *************************************************************************/
void dru_parse_read_blk(dru_dev_id_t asic,
                        uint32_t *wd_ptr,
                        uint64_t src_addr,
                        int entry_sz,
                        int n_entries,
                        int addr_step) {
#ifdef TARGET_IS_MODEL
  uint64_t data_bus_0;
  uint64_t data_bus_1;
  uint64_t addr_bus;
  int n;
  int step;
  bf_dev_family_t dev_family = lld_dev_family_get(asic);
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      step = addr_step ? 1 : 4;  // convert addr_type to step
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:

      step = addr_step;
      break;
    default:
      return;
  }
  if (0) {
    printf("dru_parse_read_blk: asic=%d, entry_sz=%d, # entries=%d\n",
           asic,
           entry_sz,
           n_entries);
  }
  addr_bus = src_addr;

  for (n = 0; n < n_entries; n++, addr_bus += step, wd_ptr += entry_sz / 4) {
    data_bus_0 = 0xeeeeeeeeeeeeeeeeull;
    data_bus_1 = 0xeeeeeeeeeeeeeeeeull;
    /* For ilist entries of not type addr_type_instruction clear bits 29:28 */
    addr_bus &= (~(0x3ull << 28ull));
    /* Detect register writes and handle them specially. */
    if ((addr_bus >> 30) & 0x3) {
      simulator_ind_read(asic, addr_bus, &data_bus_0, &data_bus_1);
    } else {
      /* Extract the pipe, stage, and 19 bit register address from the
       * tofino address. */
      uint64_t p, s, r;
      switch (dev_family) {
        uint32_t reg_addr;
        case BF_DEV_FAMILY_TOFINO:
          p = (addr_bus >> 37) & 0x3;
          s = (addr_bus >> 33) & 0xF;
          r = addr_bus & 0x7FFFF;
          /* Build a direct PCIe address from the extracted data. */
          reg_addr = (1 << 25) | (p << 23) | (s << 19) | r;
          data_bus_0 = simulator_reg_read(asic, reg_addr);
          data_bus_1 = 0;
          break;
        case BF_DEV_FAMILY_TOFINO2:
        case BF_DEV_FAMILY_TOFINO3:

          if (dev_family == BF_DEV_FAMILY_TOFINO2 ||
              dev_family == BF_DEV_FAMILY_TOFINO3) {
            p = (addr_bus >> 39) & 0x3;
            s = (addr_bus >> 34) & 0x1F;
            r = addr_bus & 0x7FFFF;
            /* Build a direct PCIe address from the extracted data. */
            reg_addr = (1 << 26) | (p << 24) | (s << 19) | r;







          }
          data_bus_0 = simulator_reg_read(asic, reg_addr);
          data_bus_1 = 0;
          break;
        default:
          break;
      }
    }
    if (0) {
      printf("::: RD BLK::: %d: %d of %d\n", asic, n, n_entries);
      printf(":::::: addr: %08" PRIx64 "\n", addr_bus);
      printf(
          ":::::: data: %08" PRIx64 " %08" PRIx64 "\n", data_bus_1, data_bus_0);
    }

    switch (entry_sz) {
      case 4:
        (*wd_ptr) = (uint32_t)(data_bus_0 & 0x0ffffffffull);
        break;
      case 8:
        *((uint64_t *)wd_ptr) = data_bus_0;
        break;
      case 16:
        *((uint64_t *)wd_ptr) = data_bus_0;
        *((uint64_t *)wd_ptr + 1) = data_bus_1;
        break;
      default:
        assert(0);
    }
  }
#else
  (void)asic;
  (void)wd_ptr;
  (void)src_addr;
  (void)entry_sz;
  (void)n_entries;
  (void)addr_step;
#endif  // TARGET_IS_MODEL
}
/*************************************************************************
 * dru_parse_tx_pkt
 *************************************************************************/
void dru_parse_tx_pkt(dru_dev_id_t asic, uint8_t *pkt_ptr, int len) {
#ifdef TARGET_IS_MODEL
  simulator_pcie_pkt_in(asic, pkt_ptr, len);
#else
  (void)asic;
  (void)pkt_ptr;
  (void)len;
#endif  // TARGET_IS_MODEL
}
