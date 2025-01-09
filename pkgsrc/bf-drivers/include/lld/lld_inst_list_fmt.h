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


#ifndef lld_inst_list_fmt_h
#define lld_inst_list_fmt_h

/* Allow the use in C++ code.  */
#ifdef __cplusplus
extern "C" {
#endif

/* This value is copied from pipe_top_level.h (pipe_top_level_pipes_address)
 * because it is statically declared there and cannot be used in some inline
 * functions below.  This duplicate defination isn't great but since this is
 * something that isn't changing we can live with it. */
/* Bit 41 is set, but the extra left shift of 4 is because all addresses are
 * "byte" address assuming 16 byte words and are right shifted by 4 to make
 * them "word" addresses. */
//#define PIPE_TOP_LEVEL_PIPES_ADDRESS 0x200000000000ull
#define PIPE_TOP_LEVEL_PIPES_ADDRESS (2ull << (40 + 4))

/* Pipe instruction opcodes. Used when 'type' = instruction (0x1) */
/* They are variable length */
#define INSTR_OPCODE_DUMP_STAT_TABLE_ENTRY 2
#define INSTR_OPCODE_DUMP_STAT_TABLE 63

#define INSTR_OPCODE_SELECT_DEST 0xfffffff
#define INSTR_OPCODE_SELECT_DEST_STAGE 0xffffffe

#define INSTR_OPCODE_PUSH_TABLE_MOVE_ADR 0x2    // 2 bits
#define INSTR_OPCODE_SALU_CNTR 0x1              // 3 bits
#define INSTR_OPCODE_NOOP 0x0                   // 5 bits
#define INSTR_OPCODE_BARRIER_LOCK 0x01          // 5 bits
#define INSTR_OPCODE_SET_TCAM_WRITE_REG 0x0d    // 7 bits
#define INSTR_OPCODE_DUMP_IDLE_TABLE_ENTRY 0xc  // 7 bits
#define INSTR_OPCODE_TCAM_COPY_DATA 0x038       // 9 bits
#define INSTR_OPCODE_ATOMIC_MOD_SRAM 0x039      // 9 bits
#define INSTR_OPCODE_ATOMIC_MOD_CSR 0x03a       // 9 bits
#define INSTR_OPCODE_POP_TABLE_MOVE_ADR 0x03c   // 9 bits
#define INSTR_OPCODE_DUMP_IDLE_TABLE 0x03e      // 9 bits
#define INSTR_OPCODE_RUN_SALU 0x03d             // 9 bits
typedef enum {
  /* . - Unused bit
   * x - Argument bit */
  PIPE_INSTR_SELECT_DEST = 0xfffffff,          /* 11....... */
  PIPE_INSTR_SELECT_DEST_STAGE = 0xffffffe,    /* 11....... */
  PIPE_INSTR_PUSH_TABLE_MOVE_ADR = 0x2 << 7,   /* 10xxxxxxx */
  PIPE_INSTR_STFUL_CNTR = 0x1 << 7,            /* 001...... */
  PIPE_INSTR_NOOP = 0x0,                       /* 00000.... */
  PIPE_INSTR_BARRIER_LOCK = 0x01 << 4,         /* 00001xxxx */
  PIPE_INSTR_DUMP_STAT_TABLE_ENTRY = 0x2 << 4, /* 00010xxxx */
  PIPE_INSTR_DUMP_IDLE_TABLE_ENTRY = 0xC << 2, /* 0001100xx */
  PIPE_INSTR_SET_TCAM_WRITE_REG = 0xD << 2,    /* 0001101.. */
  PIPE_INSTR_TCAM_COPY_DATA = 0x38,            /* 000111000 */
  PIPE_INSTR_ATOMIC_MOD_SRAM = 0x39,           /* 000111001 */
  PIPE_INSTR_ATOMIC_MOD_CSR = 0x3A,            /* 000111010 */
  PIPE_INSTR_POP_TABLE_MOVE_ADR = 0x03c,       /* 000111100 */
  PIPE_INSTR_RUN_SALU = 0x3D,                  /* 000111101 */
  PIPE_INSTR_DUMP_IDLE_TABLE = 0x3E,           /* 000111110 */
  PIPE_INSTR_DUMP_STAT_TABLE = 0x3F            /* 000111111 */
} pipe_instr_type_e;

typedef enum {
  addr_type_register = 0,
  addr_type_instruction,
  addr_type_memdata,   /* Physical read/write */
  addr_type_memdata_v, /* Virtual read/write */
} pipe_ring_addr_type_e;

typedef enum {
  pipe_virt_mem_type_stat = 0,
  pipe_virt_mem_type_meter = 1,
  pipe_virt_mem_type_sel_stful = 2,
  pipe_virt_mem_type_idle = 3
} pipe_virt_mem_type_e;

typedef enum {
  pipe_mem_type_unit_ram = 0,
  pipe_mem_type_map_ram = 1,
  pipe_mem_type_stats_deferred_access_ram = 2,
  pipe_mem_type_meter_deferred_access_ram = 3,
  pipe_mem_type_tcam = 4,


  pipe_mem_type_shadow_reg = 7,
} pipe_mem_type_t;

static inline char const *mem_type_to_str(pipe_mem_type_t t) {
  switch (t) {
    case pipe_mem_type_unit_ram:
      return "UnitRAM";
    case pipe_mem_type_map_ram:
      return "MapRAM";
    case pipe_mem_type_stats_deferred_access_ram:
      return "DefRAM-Stats";
    case pipe_mem_type_meter_deferred_access_ram:
      return "DefRAM-Meter";
    case pipe_mem_type_tcam:
      return "TCAM";






    case pipe_mem_type_shadow_reg:
      return "UnitRAM Shadow Register";
    default:
      return "Unknown";
  }
}

typedef union pipe_physical_addr_s {
  uint64_t addr;
  struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t mem_address : 10; /* Word address within <mem_id> memory */
    uint32_t
        mem_col : 4; /* Physical RAM column identifier with the pipe element */
    uint32_t
        mem_row : 4; /* Physical RAM row identifier with the pipe element */
    uint32_t mem_type : 3; /* Memory type */
    uint32_t reserved : 7; /* Must be set to 0 */
    uint32_t data_width : 2;
    /* Function of the particular memory. Always must be full data width
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b
     */
    uint32_t pipe_ring_addr_type : 2;
    uint32_t pipe_always_0 : 1;      // new, smaller stage
    uint32_t pipe_element_36_33 : 4; /* MAUID or 0xE for Parsers or 0xF for
                                        De-Parsers */
    uint32_t pipe_id_39_37 : 3;      /* PipeID */
    uint32_t pipe_41_40 : 2;         /* Always 0x01 to indicate pipe */
    uint32_t rsvd1 : 22;             /* Padding unused bits */
#else
    uint32_t rsvd1 : 22;             /* Padding unused bits */
    uint32_t pipe_41_40 : 2;         /* Always 0x01 to indicate pipe */
    uint32_t pipe_id_39_37 : 3;      /* PipeID */
    uint32_t pipe_element_36_33 : 4; /* MAUID or 0xE for Parsers or 0xF for
                                        De-Parsers */
    uint32_t pipe_always_0 : 1;      // new, smaller stage
    uint32_t pipe_ring_addr_type : 2;
    uint32_t data_width : 2;
    /* Function of the particular memory. Always must be full data width
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b
     */
    uint32_t reserved : 7; /* Must be set to 0 */
    uint32_t mem_type : 3; /* Memory type */
    uint32_t
        mem_row : 4; /* Physical RAM row identifier with the pipe element */
    uint32_t
        mem_col : 4; /* Physical RAM column identifier with the pipe element */
    uint32_t mem_address : 10; /* Word address within <mem_id> memory */
#endif
  } tof;
  struct {
    uint32_t mem_address : 10; /* Word address within <mem_id> memory */
    uint32_t mem_col : 4;      /* Physical RAM column. */
    uint32_t mem_row : 4;      /* Physical RAM row. */
    uint32_t
        mem_type : 3; /* Must be addr_type_memdata (pipe_ring_addr_type_e) */
    uint32_t reserved : 7; /* Must be set to 0 */
    uint32_t data_width : 2;
    /* Function of the particular memory. Always must be full data width
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b
     */
    uint32_t pipe_ring_addr_type : 2;
    uint32_t pipe_always_0 : 2;
    uint32_t pipe_stage : 5;
    uint32_t pipe_id : 2;
    uint32_t pipe_always_1 : 1;
    uint32_t rsvd1 : 22; /* Pad upto to 64 bits. */
  } tof2;
  struct {
    uint32_t mem_address : 10; /* Word address within <mem_id> memory */
    uint32_t mem_col : 4;      /* Physical RAM column. */
    uint32_t mem_row : 4;      /* Physical RAM row. */
    uint32_t
        mem_type : 3; /* Must be addr_type_memdata (pipe_ring_addr_type_e) */
    uint32_t reserved : 7; /* Must be set to 0 */
    uint32_t data_width : 2;
    /* Function of the particular memory. Always must be full data width
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b
     */
    uint32_t pipe_ring_addr_type : 2;
    uint32_t pipe_always_0 : 2;
    uint32_t pipe_stage : 5;
    uint32_t pipe_id : 2;
    uint32_t pipe_always_1 : 1;
    uint32_t rsvd1 : 22; /* Pad upto to 64 bits. */
  } tof3;























} pipe_physical_addr_t;

/* Virtual address definition */
typedef union pipe_full_virt_addr_t {
  uint64_t addr;
  struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t v_address : 21;          /* Address within the logical memory */
    uint32_t table_id : 4;            /* Logical table id */
    uint32_t v_mem_type : 2;          /* pipe_virt_mem_type_e */
    uint32_t zero_bit_27 : 1;         /* Always set to zero */
    uint32_t zero_bits_28_29 : 2;     /* Always set to zero */
    uint32_t pipe_ring_addr_type : 2; /* Always 3 for virtual addresses */
    uint32_t zero_bit_32 : 1;         /* Always set to zero */
    uint32_t stage_id : 4;            /* Stage id */
    uint32_t pipe_id : 2;             /* Pipe id */
    uint32_t pipe_id_2 : 1;           /* Always zero */
    uint32_t bits_41_40 : 2;          /* Always set to b'10 */
#else
    uint32_t reserved : 22;
    uint32_t bits_41_40 : 2;          /* Always set to b'10 */
    uint32_t pipe_id_2 : 1;           /* Always zero */
    uint32_t pipe_id : 2;             /* Pipe id */
    uint32_t stage_id : 4;            /* Stage id */
    uint32_t zero_bit_32 : 1;         /* Always set to zero */
    uint32_t pipe_ring_addr_type : 2; /* Always 3 for virtual addresses */
    uint32_t zero_bits_28_29 : 2;     /* Always set to zero */
    uint32_t zero_bit_27 : 1;         /* Always set to zero */
    uint32_t v_mem_type : 2;          /* pipe_virt_mem_type_e */
    uint32_t table_id : 4;            /* Logical table id */
    uint32_t v_address : 21;          /* Address within the logical memory */
#endif
  } tof;
  struct {
    uint32_t v_address : 21;          /* Address within the logical memory */
    uint32_t table_id : 4;            /* Logical table id */
    uint32_t v_mem_type : 2;          /* pipe_virt_mem_type_e */
    uint32_t zero_bit_27 : 1;         /* Always set to zero */
    uint32_t zero_bits_28_29 : 2;     /* Always set to zero */
    uint32_t pipe_ring_addr_type : 2; /* Always 3 for virtual addresses */
    uint32_t zero : 2;                /* Bits 32,33 always set to zero */
    uint32_t stage_id : 5;            /* Stage id */
    uint32_t pipe_id : 2;             /* Pipe id */
    uint32_t pipe_always_1 : 1;       /* Always set to 1 for pipe addresses */
  } tof2;
  struct {
    uint32_t v_address : 21;          /* Address within the logical memory */
    uint32_t table_id : 4;            /* Logical table id */
    uint32_t v_mem_type : 2;          /* pipe_virt_mem_type_e */
    uint32_t zero_bit_27 : 1;         /* Always set to zero */
    uint32_t zero_bits_28_29 : 2;     /* Always set to zero */
    uint32_t pipe_ring_addr_type : 2; /* Always 3 for virtual addresses */
    uint32_t zero : 2;                /* Bits 32,33 always set to zero */
    uint32_t stage_id : 5;            /* Stage id */
    uint32_t pipe_id : 2;             /* Pipe id */
    uint32_t pipe_always_1 : 1;       /* Always set to 1 for pipe addresses */
  } tof3;
} pipe_full_virt_addr_t;

typedef struct pipe_instr_common_wd0_t {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t specific : 28;
  uint32_t data_width : 2;
  uint32_t pipe_ring_addr_type : 2;
#else
  uint32_t pipe_ring_addr_type : 2;
  uint32_t data_width : 2;
  uint32_t specific : 28;
#endif
} pipe_instr_common_wd0_t;

static inline pipe_instr_type_e decode_instr_opcode(
    pipe_instr_common_wd0_t *x) {
  unsigned int opcode = (x->specific >> 19) & 0x1FF;
  /* Since the opcode is variable length the order in which the below checks
   * are done is important. */
  if (x->specific == INSTR_OPCODE_SELECT_DEST) {
    return PIPE_INSTR_SELECT_DEST;
  } else if (x->specific == INSTR_OPCODE_SELECT_DEST_STAGE) {
    return PIPE_INSTR_SELECT_DEST_STAGE;
  } else if (opcode == INSTR_OPCODE_ATOMIC_MOD_CSR) {
    return PIPE_INSTR_ATOMIC_MOD_CSR;
  } else if (opcode == INSTR_OPCODE_ATOMIC_MOD_SRAM) {
    return PIPE_INSTR_ATOMIC_MOD_SRAM;
  } else if (opcode == INSTR_OPCODE_POP_TABLE_MOVE_ADR) {
    return PIPE_INSTR_POP_TABLE_MOVE_ADR;
  } else if (opcode == INSTR_OPCODE_RUN_SALU) {
    return PIPE_INSTR_RUN_SALU;
  } else if (opcode == INSTR_OPCODE_DUMP_IDLE_TABLE) {
    return PIPE_INSTR_DUMP_IDLE_TABLE;
  } else if (opcode == INSTR_OPCODE_DUMP_STAT_TABLE) {
    return PIPE_INSTR_DUMP_STAT_TABLE;
  } else if (opcode == INSTR_OPCODE_TCAM_COPY_DATA) {
    return PIPE_INSTR_TCAM_COPY_DATA;
  } else if (opcode >> 2 == INSTR_OPCODE_SET_TCAM_WRITE_REG) {
    return PIPE_INSTR_SET_TCAM_WRITE_REG;
  } else if (opcode >> 2 == INSTR_OPCODE_DUMP_IDLE_TABLE_ENTRY) {
    return PIPE_INSTR_DUMP_IDLE_TABLE_ENTRY;
  } else if (opcode >> 4 == INSTR_OPCODE_BARRIER_LOCK) {
    return PIPE_INSTR_BARRIER_LOCK;
  } else if (opcode >> 4 == INSTR_OPCODE_DUMP_STAT_TABLE_ENTRY) {
    return PIPE_INSTR_DUMP_STAT_TABLE_ENTRY;
  } else if (opcode >> 7 == INSTR_OPCODE_PUSH_TABLE_MOVE_ADR) {
    return PIPE_INSTR_PUSH_TABLE_MOVE_ADR;
  } else if (PIPE_INSTR_STFUL_CNTR >> 7 == opcode) {
    return PIPE_INSTR_STFUL_CNTR;
  } else if (opcode >> 7 == INSTR_OPCODE_NOOP) {
    return PIPE_INSTR_NOOP;
  }
  return 0;
}

typedef struct {
  uint32_t inc_amount : 20; /* Zero based */
  uint32_t cntr : 2;
  uint32_t resvd : 2;
  uint32_t push_1_pop_0 : 1;
  uint32_t opcode : 3;     // PIPE_INSTR_STFUL_CNTR
  uint32_t data_width : 2; /* Always 0 */
  uint32_t pipe_ring_addr_type : 2;
} pipe_salu_cntr_instr_t;

typedef struct pipe_run_salu_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t instr_index : 2;
  uint32_t logical_table : 4;
  uint32_t pad0 : 13;
  uint32_t opcode : 9;     // INSTR_OPCODE_RUN_SALU
  uint32_t data_width : 2; /* Always 1 */
  uint32_t pipe_ring_addr_type : 2;
  /* Always 0x01 to indicate the instruction type in
   * pipe ring address map
   */
  struct {
    uint32_t addr : 23;
    uint32_t pad1 : 9;
  } data;
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always 0x01 to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2; /* Always 1 */
  uint32_t opcode : 9;     // INSTR_OPCODE_RUN_SALU
  uint32_t pad0 : 13;
  uint32_t logical_table : 4;
  uint32_t instr_index : 2;
  struct {
    uint32_t pad1 : 9;
    uint32_t addr : 23;
  } data;
#endif
} pipe_run_salu_instr_t;

typedef struct pipe_tcam_copy_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t num_words : 9;
  uint32_t addr_inc_dir : 1;
  uint32_t pad0 : 9;
  uint32_t opcode : 9;  // INSTR_OPCODE_TCAM_COPY_DATA
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction.
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always 0x01 to indicate the instruction type in
 * pipe ring address map
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always 0x01 to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction.
   */
  uint32_t opcode : 9;  // INSTR_OPCODE_TCAM_COPY_DATA
  uint32_t pad0 : 9;
  uint32_t addr_inc_dir : 1;
  uint32_t num_words : 9;
#endif
} pipe_tcam_copy_instr_t;

typedef struct pipe_tcam_copy_instr_data_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t dest_line_no : 9;
  uint32_t src_line_no : 9;
  uint32_t top_row : 4;
  uint32_t bottom_row : 4;
  uint32_t dest_col : 1;
  uint32_t src_col : 1;
  uint32_t pad1 : 4;
#else
  uint32_t pad1 : 4;
  uint32_t src_col : 1;
  uint32_t dest_col : 1;
  uint32_t bottom_row : 4;
  uint32_t top_row : 4;
  uint32_t src_line_no : 9;
  uint32_t dest_line_no : 9;
#endif
} pipe_tcam_copy_instr_data_t;

typedef struct pipe_set_tcam_write_reg_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t mem_address : 9; /* Word address within <mem_id> memory */
  uint32_t write_tcam : 1;
  uint32_t tcam_col : 1;
  uint32_t tcam_row : 4;
  uint32_t pad0 : 6;
  uint32_t opcode : 7;  // INSTR_OPCODE_SET_TCAM_WRITE_REG
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction.
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always 0x01 to indicate the instruction type in
 * pipe ring address map
 */
/*****************************************************
 * Operand data word-1 of the instruction list entry *
 *****************************************************/
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always 0x01 to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction.
   */
  uint32_t opcode : 7;  // INSTR_OPCODE_SET_TCAM_WRITE_REG
  uint32_t pad0 : 6;
  uint32_t tcam_row : 4;
  uint32_t tcam_col : 1;
  uint32_t write_tcam : 1;
  uint32_t mem_address : 9; /* Word address within <mem_id> memory */
#endif
} pipe_set_tcam_write_reg_instr_t;

typedef pipe_set_tcam_write_reg_instr_t pipe_tcam_invalidate_instr_t;

/* Definition of atomic modify csr instruction */
typedef struct pipe_atomic_mod_csr_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t dir : 1;   /* 0 for ig, 1 for eg */
  uint32_t start : 1; /* atomic_mod_csr start flag */
  uint32_t wide : 1;  /* Wide end instr flag */
  uint32_t pad1 : 16;
  uint32_t opcode : 9;     /* INSTR_OPCODE_ATOMIC_MOD_CSR */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate instruction type in
 * pipe ring address map.
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate instruction type in
   * pipe ring address map.
   */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t opcode : 9;     /* INSTR_OPCODE_ATOMIC_MOD_CSR */
  uint32_t pad1 : 16;
  uint32_t wide : 1;  /* Wide end instr flag */
  uint32_t start : 1; /* atomic_mod_csr start flag */
  uint32_t dir : 1;   /* 0 for ig, 1 for eg */
#endif
} pipe_atomic_mod_csr_instr_t;

/* Definition of atomic modify sram instruction */
typedef struct pipe_atomic_mod_sram_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t dir : 1; /* 0 for ig, 1 for eg */
  uint32_t pad : 18;
  uint32_t opcode : 9;     /* INSTR_OPCODE_ATOMIC_MOD_SRAM */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate instruction type in
 * pipe ring address map.
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate instruction type in
   * pipe ring address map.
   */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t opcode : 9;     /* INSTR_OPCODE_ATOMIC_MOD_SRAM */
  uint32_t pad : 18;
  uint32_t dir : 1; /* 0 for ig, 1 for eg */
#endif
} pipe_atomic_mod_sram_instr_t;

/* Definition of idle entry dump instruction */
typedef struct pipe_dump_idle_ent_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t addr : 16;  /* Entry virtual address */
  uint32_t clear : 1;  /* Clear entire word to CSR configured value */
  uint32_t tbl_id : 4; /* Stat table logical table id */
  uint32_t pad : 2;
  uint32_t opcode : 5;     /* INSTR_OPCODE_DUMP_IDLE_TABLE_ENTRY */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate instruction type in
 * pipe ring address map.
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate instruction type in
   * pipe ring address map.
   */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t opcode : 5;     /* INSTR_OPCODE_DUMP_IDLE_TABLE_ENTRY */
  uint32_t pad : 2;
  uint32_t tbl_id : 4; /* Stat table logical table id */
  uint32_t clear : 1;  /* Clear entire word to CSR configured value */
  uint32_t addr : 16;  /* Entry virtual address */
#endif
} pipe_dump_idle_ent_instr_t;

typedef struct pipe_dump_idle_table_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t tbl_id : 4;  // The logical table id
  uint32_t clear : 1;
  uint32_t pad0 : 14;
  uint32_t opcode : 9;  // INSTR_OPCODE_DUMP_IDLE_TABLE
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate the instruction type in
 * pipe ring address map
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t opcode : 9;  // INSTR_OPCODE_DUMP_IDLE_TABLE
  uint32_t pad0 : 14;
  uint32_t clear : 1;
  uint32_t tbl_id : 4;  // The logical table id
#endif
} pipe_dump_idle_table_instr_t;

typedef struct pipe_push_table_move_adr_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t s_address : 19;
  uint32_t pad0 : 1;
  uint32_t tbl_id : 4;  // The logical table id
  uint32_t pad1 : 2;
  uint32_t opcode : 2;  // INSTR_OPCODE_PUSH_TABLE_MOVE_ADR
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate the instruction type in
 * pipe ring address map
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t opcode : 2;  // INSTR_OPCODE_PUSH_TABLE_MOVE_ADR
  uint32_t pad1 : 2;
  uint32_t tbl_id : 4;  // The logical table id
  uint32_t pad0 : 1;
  uint32_t s_address : 19;
#endif
} pipe_push_table_move_adr_instr_t;

typedef struct pipe_pop_table_move_adr_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t tbl_id : 4;  // The logical table id
  uint32_t idle_max_val : 1;
  uint32_t stat_init : 1;
  uint32_t pad0 : 13;
  uint32_t opcode : 9;  // INSTR_OPCODE_POP_TABLE_MOVE_ADR
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate the instruction type in
 * pipe ring address map
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t opcode : 9;  // INSTR_OPCODE_POP_TABLE_MOVE_ADR
  uint32_t pad0 : 13;
  uint32_t stat_init : 1;
  uint32_t idle_max_val : 1;
  uint32_t tbl_id : 4;  // The logical table id
#endif
} pipe_pop_table_move_adr_instr_t;

/* Definition of stat entry dump instruction */
typedef struct pipe_dump_stat_ent_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t addr : 19;      /* Entry virtual address */
  uint32_t tbl_id : 4;     /* Stat table logical table id */
  uint32_t opcode : 5;     /* INSTR_OPCODE_DUMP_STAT_TABLE_ENTRY */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate instruction type in
 * pipe ring address map.
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate instruction type in
   * pipe ring address map.
   */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t opcode : 5;     /* INSTR_OPCODE_DUMP_STAT_TABLE_ENTRY */
  uint32_t tbl_id : 4;     /* Stat table logical table id */
  uint32_t addr : 19;      /* Entry virtual address */
#endif
} pipe_dump_stat_ent_instr_t;

/* Definition of stat table dump instruction */
typedef struct pipe_dump_stat_tbl_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t tbl_id : 4;     /* Stat table logical table id */
  uint32_t padding : 15;   /* Zero padding bits */
  uint32_t opcode : 9;     /* INSTR_OPCODE_DUMP_STAT_TABLE */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate instruction type in
 * pipe ring address map.
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate instruction type in
   * pipe ring address map.
   */
  uint32_t data_width : 2; /* Zero for this instruction */
  uint32_t opcode : 9;     /* INSTR_OPCODE_DUMP_STAT_TABLE */
  uint32_t padding : 15;   /* Zero padding bits */
  uint32_t tbl_id : 4;     /* Stat table logical table id */
#endif
} pipe_dump_stat_tbl_instr_t;

typedef enum {
  STAT_BARRIER = 0,
  IDLE_BARRIER,
  ALL_LOCK,
  ALL_UNLOCK,
  STAT_LOCK,
  STAT_UNLOCK,
  IDLE_LOCK,
  IDLE_UNLOCK
} pipe_barrier_lock_type_e;

typedef struct pipe_barrier_lock_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t lock_id : 16;
  uint32_t lock_type : 3;
  uint32_t tbl_id : 4;
  uint32_t opcode : 5;  // INSTR_OPCODE_BARRIER_LOCK
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate the instruction type in
 * pipe ring address map
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t opcode : 5;  // INSTR_OPCODE_BARRIER_LOCK
  uint32_t tbl_id : 4;
  uint32_t lock_type : 3;
  uint32_t lock_id : 16;
#endif
} pipe_barrier_lock_instr_t;

typedef struct pipe_noop_instr_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t unused : 23;
  uint32_t opcode : 5;  // INSTR_OPCODE_NOOP
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always addr_type_instruction to indicate the instruction type in
 * pipe ring address map
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always addr_type_instruction to indicate the instruction type in
   * pipe ring address map
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction. -- 0
   */
  uint32_t opcode : 5;  // INSTR_OPCODE_NOOP
  uint32_t unused : 23;
#endif
} pipe_noop_instr_t;

typedef struct dest_select_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t opcode : 28;    /* INSTR_OPCODE_SELECT_DEST=0 */
  uint32_t data_width : 2; /* Number of 32b data words in the instruction.
                            * Always 01 for the select_dest instruction */
  uint32_t pipe_ring_addr_type : 2; /* Always 0x01 to indicate the instruction
                                     * type in
                                     * pipe ring address map */
  uint32_t pipe_always_0 : 1;       // new, smaller stage
  uint32_t pipe_element_36_33 : 4;  /* MAUID or 0xE for Parsers or 0xF for
                                       De-Parsers */
  uint32_t pipe_id_39_37 : 3;       /* PipeID */
  uint32_t pipe_41_40 : 2;          /* Always 0x01 to indicate pipe */
  uint32_t pad0 : 22;               /* Padding unused bits */
#else
  uint32_t pipe_ring_addr_type : 2; /* Always 0x01 to indicate the instruction
                                     * type in
                                     * pipe ring address map */
  uint32_t data_width : 2;    /* Number of 32b data words in the instruction.
                               * Always 01 for the select_dest instruction */
  uint32_t opcode : 28;       /* INSTR_OPCODE_SELECT_DEST=0 */
  uint32_t pad0 : 22;         /* Padding unused bits */
  uint32_t pipe_41_40 : 2;    /* Always 0x01 to indicate pipe */
  uint32_t pipe_id_39_37 : 3; /* PipeID */
  uint32_t pipe_element_36_33 : 4;  /* MAUID or 0xE for Parsers or 0xF for
                                       De-Parsers */
  uint32_t pipe_always_0 : 1;       // new, smaller stage
#endif
} dest_select_t;

typedef struct dest_select_stage_s {
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t opcode : 28;    /* INSTR_OPCODE_SELECT_DEST_STAGE */
  uint32_t data_width : 2; /* Number of 32b data words in the instruction.
                            * Always 01 for the select_dest instruction */
  uint32_t pipe_ring_addr_type : 2; /* Always 0x01 to indicate the instruction
                                     * type in
                                     * pipe ring address map */
  uint32_t pipe_always_0 : 1;       // new, smaller stage
  uint32_t pipe_element_36_33 : 4;  /* MAUID or 0xE for Parsers or 0xF for
                                       De-Parsers */
  uint32_t pipe_id_39_37 : 3;       /* PipeID */
  uint32_t pipe_41_40 : 2;          /* Always 0x01 to indicate pipe */
  uint32_t pad0 : 22;               /* Padding unused bits */
#else
  uint32_t pipe_ring_addr_type : 2; /* Always 0x01 to indicate the instruction
                                     * type in
                                     * pipe ring address map */
  uint32_t data_width : 2;    /* Number of 32b data words in the instruction.
                               * Always 01 for the select_dest instruction */
  uint32_t opcode : 28;       /* INSTR_OPCODE_SELECT_DEST_STAGE */
  uint32_t pad0 : 22;         /* Padding unused bits */
  uint32_t pipe_41_40 : 2;    /* Always 0x01 to indicate pipe */
  uint32_t pipe_id_39_37 : 3; /* PipeID */

  uint32_t pipe_element_36_33 : 4; /* MAUID or 0xE for Parsers or 0xF for
                                      De-Parsers */
  uint32_t pipe_always_0 : 1;      // new, smaller stage
#endif
} dest_select_stage_t;

/* Instruction format for writing to a register in the pipe */
typedef union pipe_instr_write_reg_i_only_s {
  /*******************************************
   * Head word of the instruction list entry *
   * Instruction opcode specific operand data encoded in the head word
   *******************************************/
  struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t reg_address : 19; /* PCIe byte address of the pipe register */
    uint32_t reserved : 9;     /* Must be set to 0 */
    uint32_t data_width : 2;
    /* Number of 32b data words in the instruction list.
     * Always 01 for the write_reg instruction list entry
     */
    uint32_t pipe_ring_addr_type : 2;
/* Always 0x00 to indicate the pipe register type of access
 */
#else
    uint32_t pipe_ring_addr_type : 2;
    /* Always 0x00 to indicate the pipe register type of access
     */
    uint32_t data_width : 2;
    /* Number of 32b data words in the instruction list.
     * Always 01 for the write_reg instruction list entry
     */
    uint32_t reserved : 9;     /* Must be set to 0 */
    uint32_t reg_address : 19; /* PCIe byte address of the pipe register */
#endif
  } tf1;






} pipe_instr_write_reg_i_only_t;
/* Instruction format for writing to a register in the pipe */
typedef struct pipe_instr_write_reg_s {
  /*******************************************
   * Head word of the instruction list entry *
   * Instruction opcode specific operand data encoded in the head word
   *******************************************/
  pipe_instr_write_reg_i_only_t head;

  /*****************************************************
   * Operand data word-1 of the instruction list entry *
   *****************************************************/
  uint32_t data; /* Data for write_reg operation */
} pipe_instr_write_reg_t;

/* Instruction format for reading from a register in the pipe */
typedef struct pipe_instr_read_reg_s {
/*******************************************
 * Head word of the instruction list entry *
 * Instruction opcode specific operand data encoded in the head word
 *******************************************/
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t reg_address : 19; /* PCIe byte address of the pipe register */
  uint32_t reserved : 9;     /* Must be set to 0 */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction list.
   * Always 01 for the write_reg instruction list entry
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always 0x00 to indicate the pipe register type of access
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always 0x00 to indicate the pipe register type of access
   */
  uint32_t data_width : 2;
  /* Number of 32b data words in the instruction list.
   * Always 01 for the write_reg instruction list entry
   */
  uint32_t reserved : 9;     /* Must be set to 0 */
  uint32_t reg_address : 19; /* PCIe byte address of the pipe register */
#endif
} pipe_instr_read_reg_t;

/* Instruction format to write into a memory word */
typedef union pipe_instr_set_memdata_i_only_t {
  /*******************************************
   * Head word of the instruction list entry *
   *******************************************/
  struct {
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t mem_address : 10; /* Word address within <mem_id> memory */
    /* Physical memory row and column. */
    uint32_t mem_col : 4;
    uint32_t mem_row : 4;
    /* Memory type, see pipe_mem_type_t */
    uint32_t mem_type : 3;
    uint32_t reserved : 7; /* Must be set to 0 */
    /* Function of the particular memory. Always must be full data width
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b */
    uint32_t data_width : 2;
    /* Always 0x10 to indicate the pipe memory type of access */
    uint32_t pipe_ring_addr_type : 2;
#else
    uint32_t pipe_ring_addr_type : 2;
    uint32_t data_width : 2;
    uint32_t reserved : 7;
    uint32_t mem_type : 3;
    uint32_t mem_row : 4;
    uint32_t mem_col : 4;
    uint32_t mem_address : 10;
#endif
  } tf1;






} pipe_instr_set_memdata_i_only_t;

/* Instruction format to write into a memory word */
typedef struct pipe_instr_set_memdata_s {
  /*******************************************
   * Head word of the instruction list entry *
   *******************************************/
  pipe_instr_set_memdata_i_only_t head;
  /*****************************************************
   * Operand data word-1 of the instruction list entry *
   *****************************************************/
  uint32_t data[4]; /* Data for set_memdata instr. Limited to data_width */
} pipe_instr_set_memdata_t;

/* Instruction format to read from a memory word */
typedef struct pipe_instr_get_memdata_t {
/*******************************************
 * Head word of the instruction list entry *
 *******************************************/
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t mem_address : 10; /* Word address within <mem_id> memory */
  uint32_t mem_col : 4;      /* Physical RAM column within the memory array */
  uint32_t mem_row : 4;  /* Physical RAM row identifier with the pipe element */
  uint32_t mem_type : 3; /* Memory type */
  uint32_t reserved : 7; /* Must be set to 0 */
  uint32_t data_width : 2;          /* Always 0 for reads, no data is sent. */
  uint32_t pipe_ring_addr_type : 2; /* Set to 2 to indicate physical access. */
#else
  uint32_t pipe_ring_addr_type : 2; /* Set to 2 to indicate physical access. */
  uint32_t data_width : 2;          /* Always 0 for reads, no data is sent. */
  uint32_t reserved : 7;            /* Must be set to 0 */
  uint32_t mem_type : 3;            /* Memory type */
  uint32_t mem_row : 4; /* Physical RAM row identifier with the pipe element */
  uint32_t mem_col : 4; /* Physical RAM column within the memory array */
  uint32_t mem_address : 10; /* Word address within <mem_id> memory */
#endif
} pipe_instr_get_memdata_t;

/* Instruction format to write into a virtual memory word */
typedef struct pipe_instr_set_memdata_v_s {
  struct {
/*******************************************
 * Head word of the instruction list entry *
 *******************************************/
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t v_address : 21; /* Row address within the logical memory */
    uint32_t table_id : 4;   /* Logical instance of memory type */
    uint32_t
        v_mem_type : 2;    /* Memory type: stats, meter, stateful, idle-time */
    uint32_t reserved : 1; /* Always set to zero. */
    uint32_t data_width : 2;
    /* Function of the particular memory
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b
     */
    uint32_t pipe_ring_addr_type : 2;
/* Always 0x11 to indicate the pipe virtual memory type of access
 */
#else
    uint32_t pipe_ring_addr_type : 2;
    /* Always 0x11 to indicate the pipe virtual memory type of access
     */
    uint32_t data_width : 2;
    /* Function of the particular memory
     * 01 for 32b
     * 10 for 64b
     * 11 for 128b
     */
    uint32_t reserved : 1; /* Always set to zero. */
    uint32_t
        v_mem_type : 2;    /* Memory type: stats, meter, stateful, idle-time */
    uint32_t table_id : 4; /* Logical instance of memory type */
    uint32_t v_address : 21; /* Row address within the logical memory */
#endif
  } s;
  /*****************************************************
   * Operand data word-1 of the instruction list entry *
   *****************************************************/
  uint32_t data[4]; /* Data for set_memdata_v instr. Limited to data_width */
} pipe_instr_set_memdata_v_t;

/* Instruction format to write into a virtual memory word */
typedef struct pipe_instr_set_memdata_v_i_only_s {
/*******************************************
 * Head word of the instruction list entry *
 *******************************************/
#if __BYTE_ORDER == __LITTLE_ENDIAN
  uint32_t v_address : 21; /* Row address within the logical memory */
  uint32_t table_id : 4;   /* Logical instance of memory type */
  uint32_t v_mem_type : 2; /* Memory type: stats, meter, stateful, idle-time */
  uint32_t reserved : 1;   /* Always set to zero. */
  uint32_t data_width : 2;
  /* Function of the particular memory
   * 01 for 32b
   * 10 for 64b
   * 11 for 128b
   */
  uint32_t pipe_ring_addr_type : 2;
/* Always 0x11 to indicate the pipe virtual memory type of access
 */
#else
  uint32_t pipe_ring_addr_type : 2;
  /* Always 0x11 to indicate the pipe virtual memory type of access
   */
  uint32_t data_width : 2;
  /* Function of the particular memory
   * 01 for 32b
   * 10 for 64b
   * 11 for 128b
   */
  uint32_t reserved : 1;   /* Always set to zero. */
  uint32_t v_mem_type : 2; /* Memory type: stats, meter, stateful, idle-time */
  uint32_t table_id : 4;   /* Logical instance of memory type */
  uint32_t v_address : 21; /* Row address within the logical memory */
#endif
} pipe_instr_set_memdata_v_i_only_t;

/* Instruction format to read a virtual memory word */
typedef struct pipe_instr_get_memdata_v_s {
  struct {
/*******************************************
 * Head word of the instruction list entry *
 *******************************************/
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint32_t v_address : 21; /* Row address within the logical memory */
    uint32_t table_id : 4;   /* Logical instance of memory type */
    uint32_t
        v_mem_type : 2;    /* Memory type: stats, meter, stateful, idle-time */
    uint32_t reserved : 1; /* Always set to zero. */
    uint32_t data_width : 2; /* Always zero for a read */
    uint32_t pipe_ring_addr_type : 2;
/* Always 0x11 to indicate the pipe virtual memory type of access
 */
#else
    uint32_t pipe_ring_addr_type : 2;
    /* Always 0x11 to indicate the pipe virtual memory type of access
     */
    uint32_t data_width : 2; /* Always zero for a read */
    uint32_t reserved : 1;   /* Always set to zero. */
    uint32_t
        v_mem_type : 2;    /* Memory type: stats, meter, stateful, idle-time */
    uint32_t table_id : 4; /* Logical instance of memory type */
    uint32_t v_address : 21; /* Row address within the logical memory */
#endif
  } s;
} pipe_instr_get_memdata_v_t;

/* MACRO for a Write register instruction */
#define TOF_CONSTRUCT_WRITE_REG_INSTR(_instr_, _reg_addr_, _data_) \
  (_instr_)->head.tf1.pipe_ring_addr_type = addr_type_register;    \
  (_instr_)->head.tf1.data_width = 1;                              \
  (_instr_)->head.tf1.reserved = 0;                                \
  (_instr_)->head.tf1.reg_address = (_reg_addr_)&0x7ffff;          \
  (_instr_)->data = _data_;

/* MACRO for a Write register instruction */
#define TOF_CONSTRUCT_READ_REG_INSTR(_instr_, _reg_addr_) \
  (_instr_)->pipe_ring_addr_type = addr_type_register;    \
  (_instr_)->data_width = 0;                              \
  (_instr_)->reserved = 0;                                \
  (_instr_)->reg_address = (_reg_addr_)&0x7ffff;

#define TOF_CONSTRUCT_DEST_SELECT_INSTR(_instr_, _pipe_id_, _stage_id_) \
  (_instr_)->pipe_always_0 = 0;                                         \
  (_instr_)->pipe_element_36_33 = (_stage_id_)&0xF;                     \
  (_instr_)->pipe_id_39_37 = (_pipe_id_)&0x7;                           \
  /* 44 = 32 + 4 (to get 128 bit address) + 8 (bits for stage) */       \
  (_instr_)->pipe_41_40 = (PIPE_TOP_LEVEL_PIPES_ADDRESS >> 44);         \
  (_instr_)->pad0 = 0;                                                  \
  (_instr_)->opcode = INSTR_OPCODE_SELECT_DEST;                         \
  (_instr_)->data_width = 1;                                            \
  (_instr_)->pipe_ring_addr_type = addr_type_instruction;

#define TOF_CONSTRUCT_DEST_SELECT_STAGE_INSTR(_instr_, _stage_id_) \
  (_instr_)->pipe_always_0 = 0;                                    \
  (_instr_)->pipe_element_36_33 = (_stage_id_)&0xF;                \
  (_instr_)->pad0 = 0;                                             \
  (_instr_)->opcode = INSTR_OPCODE_SELECT_DEST_STAGE;              \
  (_instr_)->data_width = 1;                                       \
  (_instr_)->pipe_ring_addr_type = addr_type_instruction;

// Extraction functions
static inline uint8_t tf1_get_stage_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.pipe_element_36_33;
}
static inline uint8_t tf2_get_stage_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.pipe_stage;
}
static inline uint8_t tf3_get_stage_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof3.pipe_stage;
}

static inline uint8_t tf1_get_pipe_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.pipe_id_39_37;
}
static inline uint8_t tf2_get_pipe_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.pipe_id;
}
static inline uint8_t tf3_get_pipe_id_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof3.pipe_id;
}

static inline uint8_t tf1_get_row_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.mem_row;
}
static inline uint8_t tf2_get_row_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_row;
}
static inline uint8_t tf3_get_row_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof3.mem_row;
}

static inline uint8_t tf1_get_col_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.mem_col;
}
static inline uint8_t tf2_get_col_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_col;
}
static inline uint8_t tf3_get_col_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof3.mem_col;
}

static inline pipe_mem_type_t tf1_get_mem_type_from_pipe_addr(
    uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.mem_type;
}
static inline pipe_mem_type_t tf2_get_mem_type_from_pipe_addr(
    uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_type;
}
static inline pipe_mem_type_t tf3_get_mem_type_from_pipe_addr(
    uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof3.mem_type;
}

static inline uint16_t tf1_get_mem_addr_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof.mem_address;
}
static inline uint16_t tf2_get_mem_addr_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof2.mem_address;
}
static inline uint16_t tf3_get_mem_addr_from_pipe_addr(uint64_t pipe_addr) {
  pipe_physical_addr_t addr;
  addr.addr = pipe_addr;
  return addr.tof3.mem_address;
}

static inline bool is_instr(uint32_t x) {
  pipe_instr_common_wd0_t *y = (pipe_instr_common_wd0_t *)&x;
  return addr_type_instruction == y->pipe_ring_addr_type;
}
static inline bool is_lock_instr(uint32_t x) {
  if (!is_instr(x)) return false;
  uint32_t code = (x >> 19) & 0x1FF; /* Bits 27:19 */
  if ((code & 0x1F0) != 0x010)
    return false;                  /*00001xxxx is barrier/lock/unlock */
  uint32_t type = (x >> 16) & 0x7; /* Bits 18:16 are lock type. */
                                   /* 000 - Stats Barrier
                                    * 001 - Idle Barrier
                                    * 010 - Lock Stats & Idle
                                    * 011 - Unlock Stats and Idle
                                    * 100 - Lock Stats
                                    * 101 - Unlock Stats
                                    * 110 - Lock Idle
                                    * 111 - Unlock Idle */
  return (type > 1) && !(type & 1);
}
static inline bool is_unlock_instr(uint32_t x) {
  if (!is_instr(x)) return false;
  uint32_t code = (x >> 19) & 0x1FF; /* Bits 27:19 */
  if ((code & 0x1F0) != 0x010)
    return false;                  /*00001xxxx is barrier/lock/unlock */
  uint32_t type = (x >> 16) & 0x7; /* Bits 18:16 are lock type. */
                                   /* 000 - Stats Barrier
                                    * 001 - Idle Barrier
                                    * 010 - Lock Stats & Idle
                                    * 011 - Unlock Stats and Idle
                                    * 100 - Lock Stats
                                    * 101 - Unlock Stats
                                    * 110 - Lock Idle
                                    * 111 - Unlock Idle */
  return (type > 1) && (type & 1);
}
static inline bool is_csr_mod_begin(uint32_t x) {
  if (!is_instr(x)) return false;
  uint32_t code = (x >> 19) & 0x1FF; /* Bits 27:19 */
  if (code != INSTR_OPCODE_ATOMIC_MOD_CSR) return false;
  /* Bit 1 is the oddly named "start" field, not set means start. */
  return (x & 2) == 0;
}
static inline bool is_csr_mod_end(uint32_t x) {
  if (!is_instr(x)) return false;
  uint32_t code = (x >> 19) & 0x1FF; /* Bits 27:19 */
  if (code != INSTR_OPCODE_ATOMIC_MOD_CSR) return false;
  /* Bit 1 is the oddly named "start" field, set means end. */
  return (x & 2) == 2;
}
#ifdef __cplusplus
}
#endif /* C++ */

#endif  // lld_inst_list_fmt_h
