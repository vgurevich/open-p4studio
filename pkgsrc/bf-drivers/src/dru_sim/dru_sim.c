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


//
//  dru_sim.c
//
//
#define _GNU_SOURCE
#include <assert.h>
#include <bf_types/bf_types.h>
#include <dru_sim/dru_sim.h>
#include <lld/bf_dma_if.h>
#include <unistd.h>
#ifdef TARGET_IS_MODEL
#include <pthread.h>
#define bf_sys_thread_t pthread_t
#define bf_sys_thread_create(a, b, c, d) pthread_create(a, NULL, b, c)
#define bf_sys_thread_set_name(a, b) pthread_setname_np(a, b)
#else
#include <target-sys/bf_sal/bf_sys_thread.h>
#endif
#include <dvm/bf_dma_types.h>
#include <lld/lld_dr_regs.h>
#include <lld/lld_dr_regs_tof.h>
/* Needed in both fifo and tcp files */
int dru_sim_debug_mode = 0;
dru_write_dma_chnl_fn write_dma_chnl;
dru_read_dma_chnl_fn read_dma_chnl;
dru_write_vpi_link_fn write_vpi_link;
dru_push_to_dru_fn push_to_dru;
dru_push_to_model_fn push_to_model;

static int dru_emu_integ = 0;
static int dru_parallel_mode = 0;

void dru_hdl_pcie_fifo(void);

bf_dma_dr_id_t path_get_dr_info(bf_dma_dr_id_t dr, uint32_t dr_max);

extern int lld_asic_ready(dru_dev_id_t chip);
extern int lld_validate_dr_id(bf_dev_id_t dev_id, bf_dma_dr_id_t dr_id);

#ifdef TARGET_IS_MODEL
#ifndef UTEST
extern void harlyn_lld_re_init(void);
#endif  // UTEST
#endif  // TARGET_IS_MODEL
extern bool lld_dev_is_tofino(dru_dev_id_t dev_id);
extern bool lld_dev_is_tof2(dru_dev_id_t dev_id);
extern bool lld_dev_is_tof3(dru_dev_id_t dev_id);

extern bf_dev_family_t lld_dev_family_get(bf_dev_id_t dev_id);

extern void post_possible_dru_work(void);
extern void dru_parse_inst_list(dru_dev_id_t asic,
                                int dru,
                                bool is_mcast,
                                int mcast_vec,
                                uint32_t *list,
                                int len);
extern void dru_parse_inst_list_read(dru_dev_id_t asic,
                                     int dru,
                                     uint32_t *list,
                                     int len,
                                     int entry_sz,
                                     uint8_t *wr_buffer,
                                     int *wr_buf_sz);
extern void dru_parse_write_blk(dru_dev_id_t asic,
                                uint32_t *wd_ptr,
                                uint64_t dest_addr,
                                int entry_sz,
                                int n_entries,
                                bool single_mode,
                                int addr_step);
extern void dru_parse_write_blk_mcast(dru_dev_id_t asic,
                                      uint32_t *wd_ptr,
                                      uint64_t dest_addr,
                                      int entry_sz,
                                      int n_entries,
                                      int mcast_vector,
                                      bool single_mode,
                                      int addr_step);
extern void dru_parse_que_write_list(dru_dev_id_t asic,
                                     uint32_t *wd_ptr,
                                     int entry_sz,
                                     int n_entries);
extern void dru_parse_read_blk(dru_dev_id_t asic,
                               uint32_t *wd_ptr,
                               uint64_t src_addr,
                               int entry_sz,
                               int n_entries,
                               int addr_step);
extern void dru_parse_tx_pkt(dru_dev_id_t asic, uint8_t *pkt_ptr, int len);

extern dr_type_e dr_get_type(bf_dma_dr_id_t dr);

#ifdef DRU_INTF_FIFO
extern void dru_dump_pcie_log(int max);
#endif

union all_chips_dru_regs {
  Dru_rspec tof[BF_DMA_MAX_TOF_DR + 1];
  tof2_Dru_rspec tof2[BF_DMA_MAX_TOF2_DR + 1];
  tof3_Dru_rspec tof3[BF_DMA_MAX_TOF3_DR + 1];

};

struct dru_sim_ilist_addr_high {
  /* Shadow of device_select pbc pbc_pbus il_address[0-3] */
  uint32_t val[4];
};
static struct dru_sim_ilist_addr_high il_high[BF_MAX_DEV_COUNT];
static union all_chips_dru_regs simulated_dr_regs[BF_MAX_DEV_COUNT];
int svc_cnt[BF_MAX_DEV_COUNT][BF_DMA_MAX_DR];

#define NONE 0xff

void *null_parm;
bf_sys_thread_t dru_sim_thread;
bf_sys_thread_t dru_service_thread;

uint8_t rd_buffer[64 * 1024];
uint8_t wr_buffer[64 * 1024];

static int get_pbc_il_addr_reg_range(dru_dev_id_t asic,
                                     uint32_t *il_addr_min,
                                     uint32_t *il_addr_max,
                                     uint32_t *count) {
  switch (lld_dev_family_get(asic)) {
    uint32_t offset;
    case BF_DEV_FAMILY_TOFINO:
      *count = tofino_device_select_pbc_pbc_pbus_il_address_array_count;
      offset = tofino_device_select_pbc_pbc_pbus_il_address_array_index_max -
               tofino_device_select_pbc_pbc_pbus_il_address_array_index_min;
      *il_addr_min = tofino_device_select_pbc_pbc_pbus_il_address_address;
      *il_addr_max =
          tofino_device_select_pbc_pbc_pbus_il_address_address + (offset * 4);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *count = tof2_reg_device_select_pbc_pbc_pbus_il_address_array_count;
      offset = tof2_reg_device_select_pbc_pbc_pbus_il_address_array_index_max -
               tof2_reg_device_select_pbc_pbc_pbus_il_address_array_index_min;
      *il_addr_min = tof2_reg_device_select_pbc_pbc_pbus_il_address_address;
      *il_addr_max =
          tof2_reg_device_select_pbc_pbc_pbus_il_address_address + (offset * 4);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *count = tof3_reg_device_select_pbc_pbc_pbus_il_address_array_count;
      offset = tof3_reg_device_select_pbc_pbc_pbus_il_address_array_index_max -
               tof3_reg_device_select_pbc_pbc_pbus_il_address_array_index_min;
      *il_addr_min = tof3_reg_device_select_pbc_pbc_pbus_il_address_address;
      *il_addr_max =
          tof3_reg_device_select_pbc_pbc_pbus_il_address_address + (offset * 4);
      break;










    default:
      return -1;
  }
  return 0;
}

int dru_sim_is_il_addr_reg(dru_dev_id_t asic, uint32_t addr) {
  uint32_t il_addr_min, il_addr_max, count;
  if (get_pbc_il_addr_reg_range(asic, &il_addr_min, &il_addr_max, &count))
    return 0;
  return addr >= il_addr_min && addr <= il_addr_max;
}

void dru_sim_update_il_addr_reg(dru_dev_id_t asic,
                                uint32_t addr,
                                uint32_t val) {
  uint32_t count, il_addr_min, il_addr_max;
  if (get_pbc_il_addr_reg_range(asic, &il_addr_min, &il_addr_max, &count))
    return;
  if (addr >= il_addr_min && addr <= il_addr_max) {
    il_high[asic].val[(addr - il_addr_min) / count] = val;
  }
  return;
}
void dru_sim_update_il_addr_inst(dru_dev_id_t asic, int which, uint64_t val) {
  il_high[asic].val[which] = (uint32_t)val;
}
uint32_t dru_sim_get_il_addr_reg(dru_dev_id_t asic, uint32_t addr) {
  uint32_t count, il_addr_min, il_addr_max;
  if (get_pbc_il_addr_reg_range(asic, &il_addr_min, &il_addr_max, &count))
    return ~0;
  if (addr >= il_addr_min && addr <= il_addr_max) {
    return il_high[asic].val[(addr - il_addr_min) / count];
  }
  return ~0;
}
uint64_t dru_sim_get_il_addr_inst(dru_dev_id_t asic, int which) {
  return (uint64_t)il_high[asic].val[which];
}

void get_dru_regs(dru_dev_id_t asic, int dr, Dru_rspec_all **dr_regs) {
  *dr_regs = NULL;
  if (lld_validate_dr_id(asic, dr)) return;
  switch (lld_dev_family_get(asic)) {
    case BF_DEV_FAMILY_TOFINO:
      *dr_regs = (Dru_rspec_all *)(&simulated_dr_regs[asic].tof[dr]);
      break;
    case BF_DEV_FAMILY_TOFINO2:
      *dr_regs = (Dru_rspec_all *)(&simulated_dr_regs[asic].tof2[dr]);
      break;
    case BF_DEV_FAMILY_TOFINO3:
      *dr_regs = (Dru_rspec_all *)(&simulated_dr_regs[asic].tof3[dr]);
      break;





    default:
      return;
  }
  return;
}

/***************************************************************
 * dru_service_thread_entry
 *
 * Thread to handle DMA read/write between the DRU and the LLD.
 * The DRU may be linked with the LLD (no model) or it may be
 * linked with the model process.
 ***************************************************************/
extern void dru_run(void);

void *dru_service_thread_entry(void *param) {
  // emulator DMA simulation is handled
  // between harlyn and the "test-bench"
  if (!dru_emu_integ || dru_parallel_mode) {
    dru_run();
  }
  /*control never reaches here. This is just to
   * keep the compiler happy */
  (void)param;
  return NULL;
}

extern int volatile terminate_reg_thread;
/***************************************************************
 * dru_sim_thread_entry
 *
 * Thread to handle PCIe register accesses
 ***************************************************************/
void *dru_sim_thread_entry(void *param) {
  (void)param;

  while (1) {
    dru_hdl_pcie_fifo();
    if (terminate_reg_thread) {
      printf("Reg thread termination requested..\n");
      terminate_reg_thread = 0;
      sleep(1);  // give dru thread time to terminate ..
#ifdef TARGET_IS_MODEL
#ifndef UTEST
#ifndef EXM_UTEST
      printf("Reg thread calling harlyn_lld_re_init\n");
      harlyn_lld_re_init();
#endif  // EXM_UTEST
#endif  // UTEST
#endif  // TARGET_IS_MODEL
      printf("Reg thread terminating..\n");
      return NULL;
    }
    sched_yield();
  }
  return NULL;
}

dru_intf_t g_dru_intf = {0};

/** dru_init ************************************************
 *
 * Tailors the DRU interface based on mode
 ************************************************************/
void dru_init(int emu_integ, int parallel_mode) {
  dru_emu_integ = emu_integ;
  dru_parallel_mode = parallel_mode;

#ifdef DRU_INTF_SHM
  dru_init_shm(&g_dru_intf);
#endif  // DRU_INTF_SHM
}

void dru_create_service_thread(void) {
  /* create the PCIe access handling thread */
  bf_sys_thread_create(&dru_sim_thread, dru_sim_thread_entry, &null_parm, 0);
  bf_sys_thread_set_name(dru_sim_thread, "bf_dru_sim");

  /* create the DMA handling thread */
  bf_sys_thread_create(
      &dru_service_thread, dru_service_thread_entry, &null_parm, 0);
  bf_sys_thread_set_name(dru_service_thread, "bf_dru_srv");
}

/** dru_sim_cpu_to_pcie_wr *******************************************
 *
 * write one u32 from CPU to a PCIe mapped address
 *************************************************************/
void dru_sim_cpu_to_pcie_wr(dru_dev_id_t asic, uint32_t addr, uint32_t value) {
  g_dru_intf.cpu_to_pcie_wr(asic, addr, value);
}

/** dru_sim_cpu_to_pcie_rd *******************************************
 *
 * read one u32 from a PCIe mapped address
 *************************************************************/
uint32_t dru_sim_cpu_to_pcie_rd(dru_dev_id_t asic, uint32_t addr) {
  return g_dru_intf.cpu_to_pcie_rd(asic, addr);
}

/** cpu_to_pcie_ind_wr *******************************************
 *
 * write one u32 from CPU to a PCIe mapped address
 *************************************************************/
void cpu_to_pcie_ind_wr(dru_dev_id_t asic,
                        uint64_t addr,
                        uint64_t data0,
                        uint64_t data1) {
  g_dru_intf.cpu_to_pcie_ind_wr(asic, addr, data0, data1);
}

/** cpu_to_pcie_ind_rd *******************************************
 *
 * read one u32 from a PCIe mapped address
 *************************************************************/
uint32_t cpu_to_pcie_ind_rd(dru_dev_id_t asic,
                            uint64_t addr,
                            uint64_t *data0,
                            uint64_t *data1) {
  return g_dru_intf.cpu_to_pcie_ind_rd(asic, addr, data0, data1);
}

/** dru_to_pcie_dma_wr ***************************************
 *
 * write "n" u32s from the DRU simulator to CPU
 *************************************************************/
void dru_to_pcie_dma_wr(dru_dev_id_t asic,
                        uint64_t addr,
                        uint8_t *buf,
                        uint32_t n) {
  g_dru_intf.dru_to_pcie_dma_wr(asic, addr, buf, n);
}

/** dru_to_pcie_dma_rd ***************************************
 *
 * read "n" u32s from the CPU to the DRU simulator
 *************************************************************/
void dru_to_pcie_dma_rd(dru_dev_id_t asic,
                        uint64_t addr,
                        uint8_t *buf,
                        uint32_t n) {
  g_dru_intf.dru_to_pcie_dma_rd(asic, addr, buf, n);
}

/** rtn_pcie_rd_data ***************************************
 *
 * return synchronous read data
 *************************************************************/
void rtn_pcie_rd_data(pcie_msg_t *msg, uint32_t value) {
  g_dru_intf.dru_rtn_pcie_rd_data(msg, value);
}

/** dru_process_mac_stat *************************************
 *
 * return synchronous read data
 *************************************************************/
void dru_process_mac_stat(uint64_t *desc) { (void)desc; }

/** dru_process_inst_list ************************************
 *
 *
 *************************************************************/
int dru_process_inst_list(dru_dev_id_t asic, int dru, uint64_t *desc) {
  int buf_sz;
  (void)dru;  // keep compiler happy if not including parsing
  bool is_mcast;
  int mcast_vec;
  buf_sz = desc[0] >> 32ULL;
  dru_to_pcie_dma_rd(asic, desc[1], rd_buffer, buf_sz);

  // different based on list-type (r/w)
  if (desc[0] & (1ull << 5ull)) {
    int wr_buf_sz = 0, entry_sz = 0, entry_sz_code = 0;

    entry_sz_code = (desc[0] >> 6ull) & 0x7;
    entry_sz = (entry_sz_code == 0)
                   ? 4
                   : (entry_sz_code == 1) ? 8 : (entry_sz_code == 2) ? 16 : 0;
    if (entry_sz == 0) {  // error
      printf("DMA: Instruction list read : Invalid entry_sz <%d>\n",
             entry_sz_code);
      return 0;
    }
    dru_parse_inst_list_read(asic,
                             dru,
                             (uint32_t *)&rd_buffer[0],
                             buf_sz,
                             entry_sz,
                             wr_buffer,
                             &wr_buf_sz);
    if (wr_buf_sz == 0) {  // error
      printf("DMA: Instruction list read : Invalid wr_buf_sz <%d>\n",
             wr_buf_sz);
      return 0;
    }
    dru_to_pcie_dma_wr(asic, desc[2], wr_buffer, wr_buf_sz);
    return wr_buf_sz;
  } else {
    switch (lld_dev_family_get(asic)) {
      case BF_DEV_FAMILY_TOFINO:
        // mcast and mcast vector are not present for tf1
        is_mcast = false;
        mcast_vec = -1;
        break;
      case BF_DEV_FAMILY_TOFINO2:
      case BF_DEV_FAMILY_TOFINO3:
        is_mcast = (desc[0] >> 9) & 1;
        mcast_vec = (desc[0] >> 10) & 0xF;
        break;






      default:
        return 0;
    }
    dru_parse_inst_list(
        asic, dru, is_mcast, mcast_vec, (uint32_t *)&rd_buffer[0], buf_sz);
    return 0;
  }
}

/** dru_process_wr_blk ***************************************
 *
 * return synchronous read data
 *************************************************************/
void dru_process_wr_blk(dru_dev_id_t asic, uint64_t *desc) {
  bf_dev_family_t dev_family;
  int entry_sz_code = BITS64(desc[0], 7, 5);
  bool single_mode;
  bool mcast_en;
  uint32_t buf_sz;
  int stride, n_entries, mc_vector;
  uint32_t entry_sz =
      (entry_sz_code == 0)
          ? 4
          : (entry_sz_code == 1) ? 8 : (entry_sz_code == 2) ? 16 : 0;
  dev_family = lld_dev_family_get(asic);
  assert(entry_sz != 0);
  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      single_mode = false;
      mcast_en = desc[0] & (1ull << 9ull);
      mc_vector = ((desc[0] >> 10) & 0xF);
      int addr_type = BITS64(desc[0], 8, 8);
      buf_sz = (desc[0] >> 32ULL);  // * entry_sz;
      n_entries = buf_sz / entry_sz;
      // convert addr_type to step in calling functions(only tof1)
      // assigned to addr_step only for uniformity.
      stride = addr_type;
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:

      single_mode = BITS64(desc[0], 8, 8);
      mcast_en = BITS64(desc[0], 9, 9);
      if (dev_family == BF_DEV_FAMILY_TOFINO2 ||
          dev_family == BF_DEV_FAMILY_TOFINO3) {
        mc_vector = BITS64(desc[0], 13, 10);




      }
      stride = 0;
      switch (BITS64(desc[0], 16, 14)) {
        case 0:
          stride = 1;
          break;
        case 1:
          stride = 2;
          break;
        case 2:
          stride = 4;
          break;
        case 3:
          stride = 8;
          break;
        case 4:
          stride = 16;
          break;
        case 5:
          stride = 32;
          break;
      }
      uint32_t data_sz = desc[0] >> 32;
      buf_sz = single_mode ? entry_sz : data_sz;
      n_entries = data_sz / entry_sz;
      break;
    default:
      return;
  }
  dru_to_pcie_dma_rd(asic, desc[1], rd_buffer, buf_sz);

  // now implement it
  if (mcast_en) {
    dru_parse_write_blk_mcast(asic,
                              (uint32_t *)&rd_buffer[0],
                              desc[2],
                              entry_sz,
                              n_entries,
                              mc_vector,
                              single_mode,
                              stride);
  } else {
    dru_parse_write_blk(asic,
                        (uint32_t *)&rd_buffer[0],
                        desc[2],
                        entry_sz,
                        n_entries,
                        single_mode,
                        stride);
  }
}

/** dru_process_rd_blk ***************************************
 *
 * return synchronous read data
 *************************************************************/
void dru_process_rd_blk(dru_dev_id_t asic, uint64_t *desc) {
  int buf_sz, addr_step;
  int entry_sz_code = BITS64(desc[0], 7, 5);
  int entry_sz = (entry_sz_code == 0)
                     ? 4
                     : (entry_sz_code == 1) ? 8 : (entry_sz_code == 2) ? 16 : 0;
  assert(entry_sz != 0);
  bf_dev_family_t dev_family = lld_dev_family_get(asic);

  switch (dev_family) {
    case BF_DEV_FAMILY_TOFINO:
      buf_sz = (desc[0] >> 32ULL) * entry_sz;
      int addr_type = BITS64(desc[0], 8, 8);
      // convert addr_type to step in calling functions(only tof1).
      // assigned to addr_step only for uniformity.
      addr_step = addr_type;
      break;
    case BF_DEV_FAMILY_TOFINO2:
    case BF_DEV_FAMILY_TOFINO3:

      buf_sz = (desc[0] >> 32ULL) * entry_sz;
      int addr_stride = BITS64(desc[0], 10, 8);
      addr_step = 1 << addr_stride;
      if (addr_step > 32) addr_step = 0;
      break;
    default:
      return;
  }
  // now implement it
  dru_parse_read_blk(asic,
                     (uint32_t *)&rd_buffer[0],
                     desc[1],
                     entry_sz,
                     buf_sz / entry_sz,
                     addr_step);
  dru_to_pcie_dma_wr(asic, desc[2], rd_buffer, buf_sz);
}

/** dru_process_que_write_list *******************************
 *
 * return synchronous read data
 *************************************************************/
void dru_process_que_write_list(dru_dev_id_t asic, uint64_t *desc) {
  int buf_sz;
  int data_sz_code = BITS64(desc[0], 7, 5);
  int data_sz = (data_sz_code == 0)
                    ? 4
                    : (data_sz_code == 1) ? 8 : (data_sz_code == 2) ? 16 : 0;
  int entry_sz = data_sz + 8;
  // int entry_cnt = (desc[0] >> 32) / data_sz;
  int entry_cnt = (desc[0] >> 32) / entry_sz;

  buf_sz = entry_sz * entry_cnt;

  dru_to_pcie_dma_rd(asic, desc[1], rd_buffer, buf_sz);

  dru_parse_que_write_list(asic, (uint32_t *)rd_buffer, data_sz, entry_cnt);
}

/** dru_process_tx_pkt ***************************************
 *
 * return synchronous read data
 *************************************************************/
void dru_process_tx_pkt(dru_dev_id_t asic, uint64_t *desc) {
  int buf_sz;

  buf_sz = desc[0] >> 32ULL;
  dru_to_pcie_dma_rd(asic, desc[1], rd_buffer, buf_sz);
  dru_parse_tx_pkt(asic, (uint8_t *)rd_buffer, buf_sz);
}

/** next_pcie_msg ********************************************
 *
 * return synchronous read data
 *************************************************************/
pcie_msg_t static_pcie_msg;

pcie_msg_t *next_pcie_msg(void) { return g_dru_intf.dru_next_pcie_msg(); }

/** lld_dr_typ ***********************************************
 *
 * Return the type of a particular DR
 *************************************************************/

dr_type_e lld_dr_typ[] = {
    // lld_dr_fm_pkt_0,
    FM,
    // lld_dr_fm_pkt_1,
    FM,
    // lld_dr_fm_pkt_2,
    FM,
    // lld_dr_fm_pkt_3,
    FM,
    // lld_dr_fm_pkt_4,
    FM,
    // lld_dr_fm_pkt_5,
    FM,
    // lld_dr_fm_pkt_6,
    FM,
    // lld_dr_fm_pkt_7,
    FM,
    // lld_dr_fm_lrt,
    FM,
    // lld_dr_fm_idle,
    FM,
    // lld_dr_fm_learn,
    FM,
    // lld_dr_fm_diag,
    FM,
    // lld_dr_tx_pipe_inst_list_0,
    TX,
    // lld_dr_tx_pipe_inst_list_1,
    TX,
    // lld_dr_tx_pipe_inst_list_2,
    TX,
    // lld_dr_tx_pipe_inst_list_3,
    TX,
    // lld_dr_tx_pipe_write_block,
    TX,
    // lld_dr_tx_pipe_read_block,
    TX,
    // lld_dr_tx_que_write_list
    TX,
    // lld_dr_tx_pkt_0,
    TX,
    // lld_dr_tx_pkt_1,
    TX,
    // lld_dr_tx_pkt_2,
    TX,
    // lld_dr_tx_pkt_3,
    TX,
    // lld_dr_tx_mac_stat,
    TX,
    // lld_dr_rx_pkt_0,
    RX,
    // lld_dr_rx_pkt_1,
    RX,
    // lld_dr_rx_pkt_2,
    RX,
    // lld_dr_rx_pkt_3,
    RX,
    // lld_dr_rx_pkt_4,
    RX,
    // lld_dr_rx_pkt_5,
    RX,
    // lld_dr_rx_pkt_6,
    RX,
    // lld_dr_rx_pkt_7,
    RX,
    // lld_dr_rx_lrt,
    RX,
    // lld_dr_rx_idle,
    RX,
    // lld_dr_rx_learn,
    RX,
    // lld_dr_rx_diag,
    RX,
    // lld_dr_cmp_pipe_inst_list_0,
    CP,
    // lld_dr_cmp_pipe_inst_list_1,
    CP,
    // lld_dr_cmp_pipe_inst_list_2,
    CP,
    // lld_dr_cmp_pipe_inst_list_3,
    CP,
    // lld_dr_cmp_que_write_list,
    CP,
    // lld_dr_cmp_pipe_write_blk,
    CP,
    // lld_dr_cmp_pipe_read_blk,
    CP,
    // lld_dr_cmp_mac_stat,
    CP,
    // lld_dr_cmp_tx_pkt_0,
    CP,
    // lld_dr_cmp_tx_pkt_1,
    CP,
    // lld_dr_cmp_tx_pkt_2,
    CP,
    // lld_dr_cmp_tx_pkt_3,
    CP,
    // lld_dr_tx_mac_write_block,
    TX,
    // lld_dr_tx_que_write_list_1,
    TX,
    // lld_dr_tx_que_read_block_0,
    TX,
    // lld_dr_tx_que_read_block_1,
    TX,
    // lld_dr_cmp_mac_write_block,
    CP,
    // lld_dr_cmp_que_write_list_1,
    CP,
    // lld_dr_cmp_que_read_block_0,
    CP,
    // lld_dr_cmp_que_read_block_1,
    CP,
};

dr_type_e dr_get_type(bf_dma_dr_id_t dr) { return (lld_dr_typ[dr]); }

typedef struct matched_drs_t {
  bf_dma_dr_id_t eg_dr;
  bf_dma_dr_id_t ig_dr;
} matched_drs_t;

matched_drs_t matched_drs[] = {
    {lld_dr_rx_pkt_0, lld_dr_fm_pkt_0},
    {lld_dr_rx_pkt_1, lld_dr_fm_pkt_1},
    {lld_dr_rx_pkt_2, lld_dr_fm_pkt_2},
    {lld_dr_rx_pkt_3, lld_dr_fm_pkt_3},
    {lld_dr_rx_pkt_4, lld_dr_fm_pkt_4},
    {lld_dr_rx_pkt_5, lld_dr_fm_pkt_5},
    {lld_dr_rx_pkt_6, lld_dr_fm_pkt_6},
    {lld_dr_rx_pkt_7, lld_dr_fm_pkt_7},
    {lld_dr_cmp_tx_pkt_0, lld_dr_tx_pkt_0},
    {lld_dr_cmp_tx_pkt_1, lld_dr_tx_pkt_1},
    {lld_dr_cmp_tx_pkt_2, lld_dr_tx_pkt_2},
    {lld_dr_cmp_tx_pkt_3, lld_dr_tx_pkt_3},
    {lld_dr_cmp_pipe_inst_list_0, lld_dr_tx_pipe_inst_list_0},
    {lld_dr_cmp_pipe_inst_list_1, lld_dr_tx_pipe_inst_list_1},
    {lld_dr_cmp_pipe_inst_list_2, lld_dr_tx_pipe_inst_list_2},
    {lld_dr_cmp_pipe_inst_list_3, lld_dr_tx_pipe_inst_list_3},
    {lld_dr_cmp_mac_stat, lld_dr_tx_mac_stat},
    {lld_dr_cmp_pipe_write_blk, lld_dr_tx_pipe_write_block},
    {lld_dr_cmp_pipe_read_blk, lld_dr_tx_pipe_read_block},
    {lld_dr_rx_lrt, lld_dr_fm_lrt},
    {lld_dr_rx_idle, lld_dr_fm_idle},
    {lld_dr_rx_learn, lld_dr_fm_learn},
    {lld_dr_rx_diag, lld_dr_fm_diag},
    {lld_dr_cmp_que_write_list, lld_dr_tx_que_write_list},
    {lld_dr_cmp_mac_write_block, lld_dr_tx_mac_write_block},
    {lld_dr_cmp_que_write_list_1, lld_dr_tx_que_write_list_1},
    {lld_dr_cmp_que_read_block_0, lld_dr_tx_que_read_block_0},
    {lld_dr_cmp_que_read_block_1, lld_dr_tx_que_read_block_1},
};

/** \brief path_get_dr_info
 *  Return associated DR (or NONE)
 */
bf_dma_dr_id_t path_get_dr_info(bf_dma_dr_id_t dr, uint32_t dr_max) {
  unsigned int p;
  if (dr_max > sizeof(matched_drs) / sizeof(matched_drs[0])) return NONE;
  for (p = 0; p < dr_max; p++) {
    if (dr == matched_drs[p].ig_dr) return matched_drs[p].eg_dr;
    if (dr == matched_drs[p].eg_dr) return matched_drs[p].ig_dr;
  }
  return NONE;
}

int volatile terminate_reg_thread = 0;

/* DRU side PCIE pipe message handler
 */
void dru_hdl_pcie_fifo(void) {
  pcie_msg_t *msg = NULL;
  while ((msg = next_pcie_msg()) != NULL) {
    if (msg->typ == pcie_op_wr) {
      /* If talking to the Mentor emulator we need to pass
       * ALL reg writes thru the model. Harlyn will screen
       * out the ones it does not implement but will forward
       * the to the emulator */
      if (dru_emu_integ && !dru_parallel_mode) {
        push_to_model(msg->typ, msg->asic, msg->addr, msg->value);
        continue;
      }

      /* hdl write from pcie device.
       * DRU register accesses are consumed
       * by this code. All others are passed on to the
       * validation model.
       */
      if (!lld_dr_is_dru_reg(msg->asic, msg->addr)) {
        /* Handle writes to the IL address registers or push it to the model. */
        if (dru_sim_is_il_addr_reg(msg->asic, msg->addr)) {
          dru_sim_update_il_addr_reg(msg->asic, msg->addr, msg->value);
        } else {
          push_to_model(msg->typ, msg->asic, msg->addr, msg->value);
        }
        continue;
      } else {
        bf_dma_dr_id_t dr;

        dr = lld_dr_id_get(msg->asic, msg->addr);
        if (lld_dev_is_tofino(msg->asic) && (dr <= BF_DMA_MAX_TOF_DR)) {
          Dru_rspec *dr_regs = NULL;
          uint32_t dru_base = lld_dr_base_get(msg->asic, dr);

          dr_regs = &simulated_dr_regs[msg->asic].tof[dr];

          if ((msg->addr - dru_base) == offsetof(Dru_rspec, head_ptr)) {
            dr_regs->head_ptr = msg->value;  // keep wrap-bit
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) == offsetof(Dru_rspec, tail_ptr)) {
            dr_regs->tail_ptr = msg->value;  // keep wrap-bit
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, base_addr_low)) {
            dr_regs->base_addr_low = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, base_addr_high)) {
            dr_regs->base_addr_high = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, limit_addr_low)) {
            dr_regs->limit_addr_low = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, limit_addr_high)) {
            dr_regs->limit_addr_high = msg->value;
          } else if ((msg->addr - dru_base) == offsetof(Dru_rspec, size)) {
            dr_regs->size = msg->value;
          } else if ((msg->addr - dru_base) == offsetof(Dru_rspec, ctrl)) {
            dr_regs->ctrl = msg->value;
          }
        } else if (lld_dev_is_tof2(msg->asic) && (dr <= BF_DMA_MAX_TOF2_DR)) {
          tof2_Dru_rspec *dr_regs = NULL;
          uint32_t dru_base = lld_dr_base_get(msg->asic, dr);

          dr_regs = &simulated_dr_regs[msg->asic].tof2[dr];

          if ((msg->addr - dru_base) == offsetof(tof2_Dru_rspec, head_ptr)) {
            dr_regs->head_ptr = msg->value;  // keep wrap-bit
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, tail_ptr)) {
            dr_regs->tail_ptr = msg->value;  // keep wrap-bit
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, base_addr_low)) {
            dr_regs->base_addr_low = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, base_addr_high)) {
            dr_regs->base_addr_high = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, limit_addr_low)) {
            dr_regs->limit_addr_low = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, limit_addr_high)) {
            dr_regs->limit_addr_high = msg->value;
          } else if ((msg->addr - dru_base) == offsetof(tof2_Dru_rspec, size)) {
            dr_regs->size = msg->value;
          } else if ((msg->addr - dru_base) == offsetof(tof2_Dru_rspec, ctrl)) {
            dr_regs->ctrl = msg->value;
          }
        } else if (lld_dev_is_tof3(msg->asic) && (dr <= BF_DMA_MAX_TOF3_DR)) {
          tof3_Dru_rspec *dr_regs = NULL;
          uint32_t dru_base = lld_dr_base_get(msg->asic, dr);

          dr_regs = &simulated_dr_regs[msg->asic].tof3[dr];

          if ((msg->addr - dru_base) == offsetof(tof3_Dru_rspec, head_ptr)) {
            dr_regs->head_ptr = msg->value;  // keep wrap-bit
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, tail_ptr)) {
            dr_regs->tail_ptr = msg->value;  // keep wrap-bit
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, base_addr_low)) {
            dr_regs->base_addr_low = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, base_addr_high)) {
            dr_regs->base_addr_high = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, limit_addr_low)) {
            dr_regs->limit_addr_low = msg->value;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, limit_addr_high)) {
            dr_regs->limit_addr_high = msg->value;
          } else if ((msg->addr - dru_base) == offsetof(tof3_Dru_rspec, size)) {
            dr_regs->size = msg->value;
          } else if ((msg->addr - dru_base) == offsetof(tof3_Dru_rspec, ctrl)) {
            dr_regs->ctrl = msg->value;
          }































        }
      }
    } else if (msg->typ == pcie_op_rd) {
      uint32_t value;

      /* If talking to the Mentor emulator we need to pass
       * ALL reg reads thru the model. Harlyn will screen
       * out the ones it does not implement but will forward
       * the to the emulator */
      if (dru_emu_integ && !dru_parallel_mode) {
        value = push_to_model(msg->typ, msg->asic, msg->addr, 0xdeadbeef);
      }
      /* hdl read from pcie device.
       * DRU register accesses are consumed
       * by this code (if !dru_emu_integ). All others are passed on to the
       * validation model.
       */
      else if (!lld_dr_is_dru_reg(msg->asic, msg->addr)) {
        /* Handle reads to the IL address registers or pass it to the model. */
        if (dru_sim_is_il_addr_reg(msg->asic, msg->addr)) {
          value = dru_sim_get_il_addr_reg(msg->asic, msg->addr);
        } else {
          value = push_to_model(msg->typ, msg->asic, msg->addr, 0xdeadbeef);
        }

      } else {
        bf_dma_dr_id_t dr;
        dr = lld_dr_id_get(msg->asic, msg->addr);
        if (lld_dev_is_tofino(msg->asic) && (dr <= BF_DMA_MAX_TOF_DR)) {
          Dru_rspec *dr_regs = NULL;
          uint32_t dru_base_u32 = lld_dr_base_get(msg->asic, dr);
          uint64_t dru_base = (uint64_t)dru_base_u32;

          dr_regs = &simulated_dr_regs[msg->asic].tof[dr];

          if ((msg->addr - dru_base) == offsetof(Dru_rspec, head_ptr)) {
            value = dr_regs->head_ptr;
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) == offsetof(Dru_rspec, tail_ptr)) {
            value = dr_regs->tail_ptr;
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, base_addr_low)) {
            value = dr_regs->base_addr_low;
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, base_addr_high)) {
            value = dr_regs->base_addr_high;
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, limit_addr_low)) {
            value = dr_regs->limit_addr_low;
          } else if ((msg->addr - dru_base) ==
                     offsetof(Dru_rspec, limit_addr_high)) {
            value = dr_regs->limit_addr_high;
          } else if ((msg->addr - dru_base) == offsetof(Dru_rspec, size)) {
            value = dr_regs->size;
          } else if ((msg->addr - dru_base) == offsetof(Dru_rspec, ctrl)) {
            value = dr_regs->ctrl;
          } else {
            value = 0xdeadbeef;
          }
        } else if (lld_dev_is_tof2(msg->asic) && (dr <= BF_DMA_MAX_TOF2_DR)) {
          tof2_Dru_rspec *dr_regs = NULL;
          uint32_t dru_base_u32 = lld_dr_base_get(msg->asic, dr);
          uint64_t dru_base = (uint64_t)dru_base_u32;
          dr_regs = &simulated_dr_regs[msg->asic].tof2[dr];
          if ((msg->addr - dru_base) == offsetof(tof2_Dru_rspec, head_ptr)) {
            value = dr_regs->head_ptr;
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, tail_ptr)) {
            value = dr_regs->tail_ptr;
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, base_addr_low)) {
            value = dr_regs->base_addr_low;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, base_addr_high)) {
            value = dr_regs->base_addr_high;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, limit_addr_low)) {
            value = dr_regs->limit_addr_low;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof2_Dru_rspec, limit_addr_high)) {
            value = dr_regs->limit_addr_high;
          } else if ((msg->addr - dru_base) == offsetof(tof2_Dru_rspec, size)) {
            value = dr_regs->size;
          } else if ((msg->addr - dru_base) == offsetof(tof2_Dru_rspec, ctrl)) {
            value = dr_regs->ctrl;
          } else {
            value = 0xdeadbeef;
          }
        } else if (lld_dev_is_tof3(msg->asic) && (dr <= BF_DMA_MAX_TOF3_DR)) {
          tof3_Dru_rspec *dr_regs = NULL;
          uint32_t dru_base_u32 = lld_dr_base_get(msg->asic, dr);
          uint64_t dru_base = (uint64_t)dru_base_u32;
          dr_regs = &simulated_dr_regs[msg->asic].tof3[dr];
          if ((msg->addr - dru_base) == offsetof(tof3_Dru_rspec, head_ptr)) {
            value = dr_regs->head_ptr;
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, tail_ptr)) {
            value = dr_regs->tail_ptr;
            post_possible_dru_work();
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, base_addr_low)) {
            value = dr_regs->base_addr_low;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, base_addr_high)) {
            value = dr_regs->base_addr_high;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, limit_addr_low)) {
            value = dr_regs->limit_addr_low;
          } else if ((msg->addr - dru_base) ==
                     offsetof(tof3_Dru_rspec, limit_addr_high)) {
            value = dr_regs->limit_addr_high;
          } else if ((msg->addr - dru_base) == offsetof(tof3_Dru_rspec, size)) {
            value = dr_regs->size;
          } else if ((msg->addr - dru_base) == offsetof(tof3_Dru_rspec, ctrl)) {
            value = dr_regs->ctrl;
          } else {
            value = 0xdeadbeef;
          }
































        } else {
          value = 0xdeadbeef;
        }
      }
      rtn_pcie_rd_data(msg, value);
    }
  }
  // reg channel closed. terminate thread
  terminate_reg_thread = 1;
  return;
}

void dru_dump_this(dru_dev_id_t chip, int dr) {
  if (lld_dev_is_tofino(chip)) {
    if (dr > BF_DMA_MAX_TOF_DR) {
      return;
    }
    printf("%d:%2d: %4d : %4d : %5d : %x : %x : %x : %x : %x : %p\n",
           chip,
           dr,
           simulated_dr_regs[chip].tof[dr].head_ptr,
           simulated_dr_regs[chip].tof[dr].tail_ptr,
           simulated_dr_regs[chip].tof[dr].size,
           simulated_dr_regs[chip].tof[dr].ctrl,
           simulated_dr_regs[chip].tof[dr].base_addr_high,
           simulated_dr_regs[chip].tof[dr].base_addr_low,
           simulated_dr_regs[chip].tof[dr].limit_addr_high,
           simulated_dr_regs[chip].tof[dr].limit_addr_low,
           (void *)&simulated_dr_regs[chip].tof[dr]);
  } else if (lld_dev_is_tof2(chip)) {
    if (dr > BF_DMA_MAX_TOF2_DR) {
      return;
    }
    printf("%d:%2d: %4d : %4d : %5d : %x : %x : %x : %x : %x : %p\n",
           chip,
           dr,
           simulated_dr_regs[chip].tof2[dr].head_ptr,
           simulated_dr_regs[chip].tof2[dr].tail_ptr,
           simulated_dr_regs[chip].tof2[dr].size,
           simulated_dr_regs[chip].tof2[dr].ctrl,
           simulated_dr_regs[chip].tof2[dr].base_addr_high,
           simulated_dr_regs[chip].tof2[dr].base_addr_low,
           simulated_dr_regs[chip].tof2[dr].limit_addr_high,
           simulated_dr_regs[chip].tof2[dr].limit_addr_low,
           (void *)&simulated_dr_regs[chip].tof2[dr]);
  } else if (lld_dev_is_tof3(chip)) {
    printf("%d:%2d: %4d : %4d : %5d : %x : %x : %x : %x : %x : %p\n",
           chip,
           dr,
           simulated_dr_regs[chip].tof3[dr].head_ptr,
           simulated_dr_regs[chip].tof3[dr].tail_ptr,
           simulated_dr_regs[chip].tof3[dr].size,
           simulated_dr_regs[chip].tof3[dr].ctrl,
           simulated_dr_regs[chip].tof3[dr].base_addr_high,
           simulated_dr_regs[chip].tof3[dr].base_addr_low,
           simulated_dr_regs[chip].tof3[dr].limit_addr_high,
           simulated_dr_regs[chip].tof3[dr].limit_addr_low,
           (void *)&simulated_dr_regs[chip].tof3[dr]);















  }
}

void dru_dump_regs(dru_dev_id_t chip) {
  int dr;
  int dr_max;
  if (lld_dev_is_tofino(chip)) {
    dr_max = BF_DMA_MAX_TOF_DR;
  } else if (lld_dev_is_tof2(chip)) {
    dr_max = BF_DMA_MAX_TOF2_DR;
  } else if (lld_dev_is_tof3(chip)) {
    dr_max = BF_DMA_MAX_TOF3_DR;




  } else {
    return;
  }
  printf(
      "-+--+------+------+-------+---+-----------------+-----------------+-----"
      "--------\n");
  printf(
      "c:dr:   hd :   tl : size  : ct:          base   :     limit       : "
      "@addr\n");
  printf(
      "-+--+------+------+-------+---+-----------------+-----------------+-----"
      "--------\n");
  for (dr = 0; dr <= dr_max; dr++) {
    dru_dump_this(chip, dr);
  }
}

void dru_dump_svc_count() {
  dru_dev_id_t chip;
  int dr;
  int dr_max;
  printf("\nSimulated DR service counts:\n");
  for (chip = 0; chip < BF_MAX_DEV_COUNT; chip++) {
    if (lld_dev_is_tofino(chip)) {
      dr_max = BF_DMA_MAX_TOF_DR;
    } else if (lld_dev_is_tof2(chip)) {
      dr_max = BF_DMA_MAX_TOF2_DR;
    } else if (lld_dev_is_tof3(chip)) {
      dr_max = BF_DMA_MAX_TOF3_DR;




    } else {
      continue;
    }
    printf("c:dr:   count\n");
    for (dr = 0; dr <= dr_max; dr++) {
      if (svc_cnt[chip][dr]) {
        printf("%d:%d: %d\n", chip, dr, svc_cnt[chip][dr]);
      }
    }
  }
}

uint64_t dma_rds = 0;
uint64_t dma_wrs = 0;
uint64_t dma_rd_bytes = 0;
uint64_t dma_wr_bytes = 0;
