/*
 * GBA JIT Dynamic Recompiler for ESP32-P4 (RISC-V)
 * 
 * This module implements a dynamic recompiler for GBA's ARM7TDMI processor
 * targeting ESP32-P4's RISC-V processor using the SLJIT library.
 */

#ifndef JIT_CORE_H_
#define JIT_CORE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/* --------------------------------------------------------------------- */
/*  SLJIT Configuration - must be before sljitLir.h               */
/* --------------------------------------------------------------------- */

#include "sljit_src/sljitConfigEsp32P4.h"
#include "sljitLir.h"

/* --------------------------------------------------------------------- */
/*  Configuration                                                        */
/* --------------------------------------------------------------------- */

/* JIT feature flags */
#define JIT_FEATURE_L1_INSTRUCTIONS    0x01
#define JIT_FEATURE_L2_INSTRUCTIONS    0x02
#define JIT_FEATURE_THUMB              0x04
#define JIT_FEATURE_BLOCK_LINKING      0x08
#define JIT_FEATURE_REGISTER_CACHING   0x10

/* --------------------------------------------------------------------- */
/*  Data Structures                                                      */
/* --------------------------------------------------------------------- */

/* Block entry in the translation cache */
typedef struct block_entry {
    uint32_t pc;                    /* Original ARM PC */
    uint8_t *native_code;           /* Pointer to native RISC-V code */
    uint32_t code_size;             /* Size of native code in bytes */
    uint16_t exec_count;            /* Number of times executed */
    uint8_t is_thumb;               /* 1 if Thumb mode block */
    uint8_t valid;                  /* 1 if block is valid */
    uint8_t l1_only;                /* 1 if only L1 instructions */
    uint8_t flags;                  /* Block flags */
    struct block_entry *next;       /* Hash collision chain */
} block_entry_t;

/* GBA CPU state for JIT execution */
typedef struct gba_cpu_state {
    uint32_t reg[16];               /* R0-R15 */
    uint32_t cpsr;                  /* Current program status register */
    uint32_t spsr;                  /* Saved program status register */
    uint32_t cycles;                /* Current cycle count */
    uint32_t cycles_target;         /* Target cycle count */
    void *mem_base;                 /* Base pointer to GBA memory */
    
    /* Cached flags for faster access */
    uint8_t n_flag;                 /* Negative flag */
    uint8_t z_flag;                 /* Zero flag */
    uint8_t c_flag;                 /* Carry flag */
    uint8_t v_flag;                 /* Overflow flag */
} gba_cpu_state_t;

/* JIT statistics */
typedef struct jit_stats {
    uint32_t total_blocks;          /* Total compiled blocks */
    uint32_t total_compiles;        /* Total compile attempts */
    uint32_t jit_hits;              /* JIT execution hits */
    uint32_t interpreter_fallbacks; /* Fallbacks to interpreter */
    uint32_t cache_full_count;      /* Cache full events */
    uint32_t l1_instructions;       /* L1 instructions translated */
    uint32_t l2_instructions;       /* L2 instructions translated */
    uint32_t complex_instructions;  /* Complex instructions (fallback) */
    uint32_t failed_pc_count;      /* Number of failed PCs marked */
    uint32_t compile_successes;     /* Successful compilations */
    uint32_t compile_failures;      /* Failed compilations */
    uint32_t cache_hits;            /* Cache lookup hits */
    uint32_t cache_misses;          /* Cache lookup misses */
} jit_stats_t;

/* JIT configuration */
typedef struct jit_config {
    uint32_t rom_cache_size;        /* ROM translation cache size */
    uint32_t ram_cache_size;        /* RAM translation cache size */
    uint32_t block_hash_size;       /* Block hash table size */
    uint32_t max_block_instructions;/* Max instructions per block */
    uint8_t enable_l1;              /* Enable L1 translation */
    uint8_t enable_l2;              /* Enable L2 translation */
    uint8_t enable_thumb;           /* Enable Thumb translation */
    uint8_t enable_stats;           /* Enable statistics */
} jit_config_t;

/* --------------------------------------------------------------------- */
/*  ARM Instruction Types                                                */
/* --------------------------------------------------------------------- */

typedef enum {
    INSN_TYPE_DATA_PROC = 0,        /* Data processing */
    INSN_TYPE_LOAD_STORE,           /* Load/store */
    INSN_TYPE_BRANCH,               /* Branch */
    INSN_TYPE_BLOCK_TRANSFER,       /* Block data transfer */
    INSN_TYPE_SWI,                  /* Software interrupt */
    INSN_TYPE_COPROC,               /* Coprocessor */
    INSN_TYPE_UNKNOWN               /* Unknown/unsupported */
} insn_type_t;

/* ARM instruction decoded info */
typedef struct arm_insn {
    insn_type_t type;               /* Instruction type */
    uint8_t cond;                   /* Condition code */
    uint8_t opcode;                 /* Opcode field */
    uint8_t rd;                     /* Destination register */
    uint8_t rn;                     /* First operand register */
    uint8_t rm;                     /* Second operand register */
    uint32_t imm;                   /* Immediate value */
    uint8_t shift_type;             /* Shift type */
    uint8_t shift_imm;              /* Shift immediate */
    bool is_imm;                    /* Is immediate operand */
    bool set_flags;                 /* Set condition flags */
    bool is_load;                   /* Is load (vs store) */
    bool is_writeback;              /* Writeback to base */
    bool is_pre_index;              /* Pre-indexed addressing */
    bool is_link;                   /* Link (BL) */
    bool is_thumb;                  /* Thumb instruction */
    int32_t offset;                 /* Offset value */
} arm_insn_t;

/* --------------------------------------------------------------------- */
/*  Translation Levels                                                   */
/* --------------------------------------------------------------------- */

typedef enum {
    TRANS_LEVEL_NONE = 0,           /* Cannot translate */
    TRANS_LEVEL_L1,                 /* Simple instructions */
    TRANS_LEVEL_L2,                 /* Medium instructions */
    TRANS_LEVEL_L3,                 /* Complex instructions */
    TRANS_LEVEL_THUMB               /* Thumb instructions */
} trans_level_t;

/* --------------------------------------------------------------------- */
/*  Core API Functions                                                   */
/* --------------------------------------------------------------------- */

/**
 * Initialize the JIT system
 * @param config Optional configuration (NULL for defaults)
 * @return 0 on success, negative on error
 */
int jit_init(const jit_config_t *config);

/**
 * Deinitialize the JIT system
 */
void jit_deinit(void);

/**
 * Reset the JIT system (clear all caches)
 */
void jit_reset(void);

/**
 * Flush the translation cache
 */
void jit_flush_cache(void);

/**
 * Look up a compiled block
 * @param pc ARM program counter
 * @param is_thumb Thumb mode flag
 * @return Block entry or NULL if not found
 */
block_entry_t *jit_block_lookup(uint32_t pc, bool is_thumb);

/**
 * Compile a block of ARM code
 * @param pc Starting ARM program counter
 * @param is_thumb Thumb mode flag
 * @return Compiled block entry or NULL on failure
 */
block_entry_t *jit_block_compile(uint32_t pc, bool is_thumb);

/**
 * Execute a compiled block
 * @param block Block to execute
 * @param cpu CPU state
 * @return New PC value
 */
uint32_t jit_execute_block(block_entry_t *block, gba_cpu_state_t *cpu);

/**
 * Get JIT statistics
 * @param stats Statistics structure to fill
 */
void jit_get_stats(jit_stats_t *stats);

/**
 * Print JIT statistics to log
 */
void jit_print_stats(void);

/**
 * Check if an instruction can be translated
 * @param opcode ARM opcode
 * @param is_thumb Thumb mode flag
 * @return Translation level
 */
trans_level_t jit_can_translate(uint32_t opcode, bool is_thumb);

/**
 * Check if an ARM instruction is L1 (simple) translatable
 * @param opcode ARM opcode (32-bit)
 * @return true if L1 translatable
 */
bool is_l1_instruction(uint32_t opcode);

/**
 * Check if a Thumb instruction is L1 (simple) translatable
 * @param opcode Thumb opcode (16-bit)
 * @return true if L1 translatable
 */
bool is_l1_thumb_instruction(uint16_t opcode);

/**
 * Mark a PC as failed translation (will not retry)
 * @param pc ARM program counter
 * @param is_thumb Thumb mode flag
 */
void jit_mark_pc_failed(uint32_t pc, bool is_thumb);

/**
 * Check if a PC has been marked as failed
 * @param pc ARM program counter
 * @param is_thumb Thumb mode flag
 * @return true if PC is marked as failed
 */
bool jit_is_pc_failed(uint32_t pc, bool is_thumb);

/* --------------------------------------------------------------------- */
/*  Memory Access Functions (to be provided by GBA core)                 */
/* --------------------------------------------------------------------- */

/* These functions must be implemented by the GBA emulator core */
extern uint32_t gba_mem_read32(uint32_t addr);
extern void gba_mem_write32(uint32_t addr, uint32_t value);
extern uint16_t gba_mem_read16(uint32_t addr);
extern void gba_mem_write16(uint32_t addr, uint16_t value);
extern uint8_t gba_mem_read8(uint32_t addr);
extern void gba_mem_write8(uint32_t addr, uint8_t value);

/* --------------------------------------------------------------------- */
/*  External Data                                                        */
/* --------------------------------------------------------------------- */

extern block_entry_t **g_block_hash;
extern uint8_t *g_rom_cache;
extern uint8_t *g_rom_cache_ptr;
extern uint8_t *g_ram_cache;
extern uint8_t *g_ram_cache_ptr;
extern jit_stats_t g_jit_stats;
extern jit_config_t g_jit_config;

#ifdef __cplusplus
}
#endif

#endif /* JIT_CORE_H_ */
