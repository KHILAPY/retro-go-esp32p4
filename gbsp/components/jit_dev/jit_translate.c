/*
 * ARM Instruction Translator for GBA JIT
 * 
 * Translates ARM7TDMI instructions to RISC-V using SLJIT.
 */

#include "jit_core.h"
#include "esp_log.h"
#include <string.h>
#include <stddef.h>

static const char *TAG = "JIT_TRANS";

extern jit_stats_t g_jit_stats;
extern jit_config_t g_jit_config;

extern bool is_l1_instruction(uint32_t opcode);
extern bool is_block_terminator(uint32_t opcode);
extern void decode_arm_instruction(uint32_t opcode, arm_insn_t *insn);

extern uint32_t gba_mem_read32(uint32_t addr);
extern void gba_mem_write32(uint32_t addr, uint32_t value);
extern uint16_t gba_mem_read16(uint32_t addr);
extern void gba_mem_write16(uint32_t addr, uint16_t value);
extern uint8_t gba_mem_read8(uint32_t addr);
extern void gba_mem_write8(uint32_t addr, uint8_t value);

static uint64_t jit_umull64(uint32_t a, uint32_t b)
{
    return (uint64_t)a * (uint64_t)b;
}

static uint64_t jit_smull64(int32_t a, int32_t b)
{
    return (int64_t)a * (int64_t)b;
}

static uint64_t jit_umlal64(uint32_t lo, uint32_t hi, uint32_t a, uint32_t b)
{
    uint64_t acc = ((uint64_t)hi << 32) | lo;
    return acc + (uint64_t)a * (uint64_t)b;
}

static uint64_t jit_smlal64(int32_t lo, int32_t hi, int32_t a, int32_t b)
{
    int64_t acc = ((int64_t)hi << 32) | (uint32_t)lo;
    return acc + (int64_t)a * (int64_t)b;
}

/* --------------------------------------------------------------------- */
/*  Conditional Execution Support                                        */
/* --------------------------------------------------------------------- */

/* Flag field offsets in gba_cpu_state_t - flags are stored as separate uint8_t fields */
#define N_FLAG_OFFSET  offsetof(gba_cpu_state_t, n_flag)
#define Z_FLAG_OFFSET  offsetof(gba_cpu_state_t, z_flag)
#define C_FLAG_OFFSET  offsetof(gba_cpu_state_t, c_flag)
#define V_FLAG_OFFSET  offsetof(gba_cpu_state_t, v_flag)

typedef enum {
    ARM_COND_EQ = 0x0, ARM_COND_NE = 0x1, ARM_COND_CS = 0x2,
    ARM_COND_CC = 0x3, ARM_COND_MI = 0x4, ARM_COND_PL = 0x5,
    ARM_COND_VS = 0x6, ARM_COND_VC = 0x7, ARM_COND_HI = 0x8,
    ARM_COND_LS = 0x9, ARM_COND_GE = 0xA, ARM_COND_LT = 0xB,
    ARM_COND_GT = 0xC, ARM_COND_LE = 0xD, ARM_COND_AL = 0xE
} arm_cond_t;

static struct sljit_jump *emit_condition_check(struct sljit_compiler *C, uint8_t cond)
{
    struct sljit_jump *jump = NULL;
    
    switch (cond) {
        case ARM_COND_EQ: /* Z==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_NE: /* Z==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_CS: /* C==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_CC: /* C==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_MI: /* N==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_PL: /* N==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_VS: /* V==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_VC: /* V==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_HI: /* C==1 && Z==0 */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 1);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        case ARM_COND_LS: /* C==0 || Z==1 */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        case ARM_COND_GE: /* N==V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_R3, 0);
            }
            break;
        case ARM_COND_LT: /* N!=V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_R3, 0);
            }
            break;
        case ARM_COND_GT: /* Z==0 && N==V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R4, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R4, 0);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        case ARM_COND_LE: /* Z==1 || N!=V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R4, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R4, 0);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        default:
            break;
    }
    
    return jump;
}

static struct sljit_jump *emit_condition_check_inverse(struct sljit_compiler *C, uint8_t cond)
{
    struct sljit_jump *jump = NULL;
    
    switch (cond) {
        case ARM_COND_EQ: /* Z==1, inverse: Z==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_NE: /* Z==0, inverse: Z==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_CS: /* C==1, inverse: C==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_CC: /* C==0, inverse: C==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_MI: /* N==1, inverse: N==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_PL: /* N==0, inverse: N==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_VS: /* V==1, inverse: V==0 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_VC: /* V==0, inverse: V==1 */
            sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                           SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
            jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            break;
        case ARM_COND_HI: /* C==1 && Z==0, inverse: C==0 || Z==1 */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        case ARM_COND_LS: /* C==0 || Z==1, inverse: C==1 && Z==0 */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 1);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        case ARM_COND_GE: /* N==V, inverse: N!=V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_R3, 0);
            }
            break;
        case ARM_COND_LT: /* N!=V, inverse: N==V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_R3, 0);
            }
            break;
        case ARM_COND_GT: /* Z==0 && N==V, inverse: Z==1 || N!=V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R4, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R4, 0);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        case ARM_COND_LE: /* Z==1 || N!=V, inverse: Z==0 && N==V */
            {
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R2, 0, 
                               SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                               SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET);
                sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R4, 0, 
                               SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET);
                sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_R4, 0);
                sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_R3, 0);
                jump = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL, SLJIT_R2, 0, SLJIT_IMM, 0);
            }
            break;
        default:
            break;
    }
    
    return jump;
}

/* --------------------------------------------------------------------- */
/*  Helper Functions                                                     */
/* --------------------------------------------------------------------- */

__attribute__((unused))
static sljit_s32 arm_reg_to_sljit(int arm_reg)
{
    if (arm_reg == 15) return SLJIT_R0;
    if (arm_reg >= 0 && arm_reg <= 14) {
        return SLJIT_MEM1(SLJIT_S0) + offsetof(gba_cpu_state_t, reg[arm_reg]);
    }
    return SLJIT_R0;
}

static void emit_load_reg(struct sljit_compiler *C, sljit_s32 dst, int arm_reg)
{
    if (arm_reg == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, dst, 0, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[15]));
    } else {
        sljit_emit_op1(C, SLJIT_MOV32, dst, 0, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[arm_reg]));
    }
}

static void emit_store_reg(struct sljit_compiler *C, int arm_reg, sljit_s32 src)
{
    if (arm_reg == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[15]),
                       src, 0);
    } else {
        sljit_emit_op1(C, SLJIT_MOV32, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[arm_reg]),
                       src, 0);
    }
}

static void emit_update_flags_nz_simple(struct sljit_compiler *C, sljit_s32 result_reg)
{
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *not_negative = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                                      SLJIT_R4, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_n_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_negative, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_n_clear, sljit_emit_label(C));
    
    struct sljit_jump *not_zero = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                                  result_reg, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_z_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_zero, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_z_clear, sljit_emit_label(C));
}

static void emit_update_flags_add(struct sljit_compiler *C, sljit_s32 result_reg,
                                   sljit_s32 op1_reg, sljit_s32 op2_reg)
{
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *not_negative = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                                      SLJIT_R4, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_n_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_negative, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_n_clear, sljit_emit_label(C));
    
    struct sljit_jump *not_zero = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                                  result_reg, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_z_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_zero, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_z_clear, sljit_emit_label(C));
    
    struct sljit_jump *no_carry = sljit_emit_cmp(C, SLJIT_32 | SLJIT_LESS,
                                                  op1_reg, 0, op2_reg, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_c_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(no_carry, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_c_clear, sljit_emit_label(C));
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, op1_reg, 0, SLJIT_IMM, 0x80000000);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R5, 0, op2_reg, 0, SLJIT_IMM, 0x80000000);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R4, 0, SLJIT_R4, 0, SLJIT_R5, 0);
    struct sljit_jump *no_overflow_1 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                                       SLJIT_R4, 0, SLJIT_IMM, 0);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R5, 0, op1_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *no_overflow_2 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                                       SLJIT_R4, 0, SLJIT_R5, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_v_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(no_overflow_2, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_v_clear, sljit_emit_label(C));
    sljit_set_label(no_overflow_1, sljit_emit_label(C));
}

static void emit_update_flags_sub(struct sljit_compiler *C, sljit_s32 result_reg,
                                   sljit_s32 op1_reg, sljit_s32 op2_reg)
{
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *not_negative = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                                      SLJIT_R4, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_n_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_negative, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_n_clear, sljit_emit_label(C));
    
    struct sljit_jump *not_zero = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                                  result_reg, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_z_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_zero, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_z_clear, sljit_emit_label(C));
    
    struct sljit_jump *no_carry = sljit_emit_cmp(C, SLJIT_32 | SLJIT_LESS,
                                                  op1_reg, 0, op2_reg, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_c_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(no_carry, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_c_clear, sljit_emit_label(C));
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, op1_reg, 0, SLJIT_IMM, 0x80000000);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R5, 0, op2_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *no_overflow_1 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                                       SLJIT_R4, 0, SLJIT_R5, 0);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *no_overflow_2 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                                       SLJIT_R4, 0, SLJIT_R5, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_v_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(no_overflow_2, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_v_clear, sljit_emit_label(C));
    sljit_set_label(no_overflow_1, sljit_emit_label(C));
}

static void emit_update_flags_sub_imm(struct sljit_compiler *C, sljit_s32 result_reg,
                                       sljit_s32 op1_reg, uint32_t imm_val)
{
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *not_negative = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                                      SLJIT_R4, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_n_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_negative, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), N_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_n_clear, sljit_emit_label(C));
    
    struct sljit_jump *not_zero = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                                  result_reg, 0, SLJIT_IMM, 0);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_z_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(not_zero, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), Z_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_z_clear, sljit_emit_label(C));
    
    struct sljit_jump *no_carry = sljit_emit_cmp(C, SLJIT_32 | SLJIT_LESS,
                                                  op1_reg, 0, SLJIT_IMM, imm_val);
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_c_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(no_carry, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_c_clear, sljit_emit_label(C));
    
    uint32_t op2_sign = imm_val & 0x80000000;
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, op1_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *no_overflow_1;
    if (op2_sign) {
        no_overflow_1 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                        SLJIT_R4, 0, SLJIT_IMM, 0);
    } else {
        no_overflow_1 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                        SLJIT_R4, 0, SLJIT_IMM, 0);
    }
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R4, 0, result_reg, 0, SLJIT_IMM, 0x80000000);
    struct sljit_jump *no_overflow_2;
    if (op2_sign) {
        no_overflow_2 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_EQUAL,
                                        SLJIT_R4, 0, SLJIT_IMM, 0);
    } else {
        no_overflow_2 = sljit_emit_cmp(C, SLJIT_32 | SLJIT_NOT_EQUAL,
                                        SLJIT_R4, 0, SLJIT_IMM, 0);
    }
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET,
                   SLJIT_IMM, 1);
    struct sljit_jump *skip_v_clear = sljit_emit_jump(C, SLJIT_JUMP);
    sljit_set_label(no_overflow_2, sljit_emit_label(C));
    sljit_emit_op1(C, SLJIT_MOV_U8, 
                   SLJIT_MEM1(SLJIT_S0), V_FLAG_OFFSET,
                   SLJIT_IMM, 0);
    sljit_set_label(skip_v_clear, sljit_emit_label(C));
    sljit_set_label(no_overflow_1, sljit_emit_label(C));
}

/* --------------------------------------------------------------------- */
/*  Instruction Translation Functions                                    */
/* --------------------------------------------------------------------- */

static int translate_add_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_add_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_sub_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_sub_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_mov_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_mov_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    
    if (rd == 15) {
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        return 1;
    }
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_and_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_and_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_ands_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_orr_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_orr_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

#define translate_orrs_imm translate_orr_imm

__attribute__((unused))
static int translate_eor_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_eor_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

#define translate_eors_imm translate_eor_imm

__attribute__((unused))
static int translate_bic_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    
    if (rd == 15) {
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        return 1;
    }
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_mvn_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    
    if (rd == 15) {
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        return 1;
    }
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_mvn_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, imm_val);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    
    if (rd == 15) {
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        return 1;
    }
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

#define translate_mvns_imm translate_mvn_imm

static int translate_bic_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, imm_val);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

#define translate_bics_imm translate_bic_imm

/* --------------------------------------------------------------------- */
/*  Flag-setting Instruction Variants (S bit = 1)                        */
/* --------------------------------------------------------------------- */

static int translate_adds_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_adds_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_subs_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_subs_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_ands_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_orrs_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_eors_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_bics_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    if (rd == 15) {
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        return 1;
    }
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_movs_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, imm_val);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_movs_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    
    if (rd == 15) {
        emit_update_flags_nz_simple(C, SLJIT_R0);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        return 1;
    }
    
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_mvns_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_rsbs_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, imm_val);
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_rsbs_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_cmp_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_update_flags_sub_imm(C, SLJIT_R0, SLJIT_R1, imm_val);
    
    return 0;
}

static int translate_cmn_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_tst_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_tst_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_teq_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_IMM, imm_val);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_teq_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_cmp_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_update_flags_sub(C, SLJIT_R0, SLJIT_R1, SLJIT_R2);
    
    return 0;
}

static int translate_cmn_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    emit_update_flags_add(C, SLJIT_R0, SLJIT_R1, SLJIT_R2);
    
    return 0;
}

/* --------------------------------------------------------------------- */
/*  Load/Store Instructions                                              */
/* --------------------------------------------------------------------- */

static int translate_ldr_imm(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int offset = opcode & 0xFFF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    
    if (rn == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, pc + 8 + offset);
    } else {
        emit_load_reg(C, SLJIT_R0, rn);
        if (pre_index) {
            if (up) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
            } else {
                sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
            }
        }
    }
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    if (writeback && !pre_index && rn != 15) {
        emit_load_reg(C, SLJIT_R0, rn);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

static int translate_str_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int offset = opcode & 0xFFF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    
    emit_load_reg(C, SLJIT_R0, rn);
    
    if (pre_index) {
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        }
    }
    
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
    
    if (writeback && !pre_index) {
        emit_load_reg(C, SLJIT_R0, rn);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

static int translate_ldr_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    
    if (pre_index) {
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        }
    }
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    if (writeback && !pre_index) {
        emit_load_reg(C, SLJIT_R0, rn);
        emit_load_reg(C, SLJIT_R2, rm);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

static int translate_str_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    
    if (pre_index) {
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        }
    }
    
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
    
    if (writeback && !pre_index) {
        emit_load_reg(C, SLJIT_R0, rn);
        emit_load_reg(C, SLJIT_R2, rm);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

static int translate_ldrh_imm(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int offset = ((opcode >> 8) & 0xF) << 4 | (opcode & 0xF);
    
    if (rn == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, pc + 8 + offset);
    } else {
        emit_load_reg(C, SLJIT_R0, rn);
        sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    }
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read16));
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_strh_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int offset = ((opcode >> 8) & 0xF) << 4 | (opcode & 0xF);
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write16));
    
    return 0;
}

static int translate_ldrb_imm(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int offset = opcode & 0xFFF;
    
    if (rn == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, pc + 8 + offset);
    } else {
        emit_load_reg(C, SLJIT_R0, rn);
        sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    }
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read8));
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_strb_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int offset = opcode & 0xFFF;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write8));
    
    return 0;
}

static int translate_thumb_ldrh_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t offset = (opcode & 0x7C0) >> 4;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read16));
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_strh_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t offset = (opcode & 0x7C0) >> 4;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write16));
    
    return 0;
}

static int translate_ldrsb_imm(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    uint32_t offset = opcode & 0xFFF;
    
    if (rn == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, pc + 8 + offset);
    } else {
        emit_load_reg(C, SLJIT_R0, rn);
        if (pre_index) {
            if (up) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
            } else {
                sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
            }
        }
    }
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read8));
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 24);
    sljit_emit_op2(C, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 24);
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    if (writeback && !pre_index && rn != 15) {
        emit_load_reg(C, SLJIT_R0, rn);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

static int translate_ldrsh_imm(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    uint32_t offset = opcode & 0xFFF;
    
    if (rn == 15) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, pc + 8 + offset);
    } else {
        emit_load_reg(C, SLJIT_R0, rn);
        if (pre_index) {
            if (up) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
            } else {
                sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
            }
        }
    }
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read16));
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 16);
    sljit_emit_op2(C, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 16);
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    if (writeback && !pre_index && rn != 15) {
        emit_load_reg(C, SLJIT_R0, rn);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

/* --------------------------------------------------------------------- */
/*  Block Data Transfer (LDM/STM)                                        */
/* --------------------------------------------------------------------- */

static int count_bits(uint16_t mask)
{
    int count = 0;
    while (mask) {
        count += mask & 1;
        mask >>= 1;
    }
    return count;
}

static int translate_ldm(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    uint16_t reg_list = opcode & 0xFFFF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    int num_regs = count_bits(reg_list);
    int has_pc = (reg_list >> 15) & 1;
    int offset = 0;
    
    emit_load_reg(C, SLJIT_R0, rn);
    
    if (!up) {
        sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
    }
    
    for (int i = 0; i < 16; i++) {
        if ((reg_list >> i) & 1) {
            if (pre_index) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 4);
            }
            
            sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
            sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                             SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
            
            if (i != 15) {
                emit_store_reg(C, i, SLJIT_R0);
            } else {
                sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFE);
                emit_store_reg(C, 15, SLJIT_R0);
                sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
            }
            
            if (!pre_index) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 4);
            }
            offset += 4;
        }
    }
    
    if (writeback && !has_pc) {
        emit_load_reg(C, SLJIT_R0, rn);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return has_pc ? 1 : 0;
}

static int translate_stm(struct sljit_compiler *C, uint32_t opcode)
{
    int rn = (opcode >> 16) & 0xF;
    uint16_t reg_list = opcode & 0xFFFF;
    int pre_index = (opcode >> 24) & 1;
    int up = (opcode >> 23) & 1;
    int writeback = (opcode >> 21) & 1;
    int num_regs = count_bits(reg_list);
    
    emit_load_reg(C, SLJIT_R0, rn);
    
    if (!up) {
        sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
    }
    
    for (int i = 0; i < 16; i++) {
        if ((reg_list >> i) & 1) {
            if (pre_index) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 4);
            }
            
            sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
            emit_load_reg(C, SLJIT_R2, i);
            sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                             SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
            
            if (!pre_index) {
                sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 4);
            }
        }
    }
    
    if (writeback) {
        emit_load_reg(C, SLJIT_R0, rn);
        if (up) {
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
        } else {
            sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
        }
        emit_store_reg(C, rn, SLJIT_R0);
    }
    
    return 0;
}

/* --------------------------------------------------------------------- */
/*  Multiply Instructions                                                */
/* --------------------------------------------------------------------- */

static int translate_mul(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 16) & 0xF;
    int rs = (opcode >> 8) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    
    sljit_emit_op2(C, SLJIT_MUL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_mla(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 16) & 0xF;
    int rn = (opcode >> 12) & 0xF;
    int rs = (opcode >> 8) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    emit_load_reg(C, SLJIT_R2, rn);
    
    sljit_emit_op2(C, SLJIT_MUL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
    
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_umull(struct sljit_compiler *C, uint32_t opcode)
{
    int rdhi = (opcode >> 16) & 0xF;
    int rdlo = (opcode >> 12) & 0xF;
    int rs = (opcode >> 8) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2(W, 32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(jit_umull64));
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R0, 0, SLJIT_IMM, 32);
    
    emit_store_reg(C, rdlo, SLJIT_R2);
    emit_store_reg(C, rdhi, SLJIT_R3);
    
    return 0;
}

static int translate_smull(struct sljit_compiler *C, uint32_t opcode)
{
    int rdhi = (opcode >> 16) & 0xF;
    int rdlo = (opcode >> 12) & 0xF;
    int rs = (opcode >> 8) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2(W, 32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(jit_smull64));
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R0, 0, SLJIT_IMM, 32);
    
    emit_store_reg(C, rdlo, SLJIT_R2);
    emit_store_reg(C, rdhi, SLJIT_R3);
    
    return 0;
}

static int translate_umlal(struct sljit_compiler *C, uint32_t opcode)
{
    int rdhi = (opcode >> 16) & 0xF;
    int rdlo = (opcode >> 12) & 0xF;
    int rs = (opcode >> 8) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rdlo);
    emit_load_reg(C, SLJIT_R1, rdhi);
    emit_load_reg(C, SLJIT_R2, rm);
    emit_load_reg(C, SLJIT_R3, rs);
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(jit_umlal64));
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R0, 0, SLJIT_IMM, 32);
    
    emit_store_reg(C, rdlo, SLJIT_R2);
    emit_store_reg(C, rdhi, SLJIT_R3);
    
    return 0;
}

static int translate_smlal(struct sljit_compiler *C, uint32_t opcode)
{
    int rdhi = (opcode >> 16) & 0xF;
    int rdlo = (opcode >> 12) & 0xF;
    int rs = (opcode >> 8) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rdlo);
    emit_load_reg(C, SLJIT_R1, rdhi);
    emit_load_reg(C, SLJIT_R2, rm);
    emit_load_reg(C, SLJIT_R3, rs);
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS4(W, 32, 32, 32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(jit_smlal64));
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R0, 0, SLJIT_IMM, 32);
    
    emit_store_reg(C, rdlo, SLJIT_R2);
    emit_store_reg(C, rdhi, SLJIT_R3);
    
    return 0;
}

__attribute__((unused))
static int translate_lsl_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_lsls_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_lsr_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    if (shift_imm == 0) shift_imm = 32;
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_lsrs_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    if (shift_imm == 0) shift_imm = 32;
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_asr_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    if (shift_imm == 0) shift_imm = 32;
    sljit_emit_op2(C, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_asrs_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    if (shift_imm == 0) shift_imm = 32;
    sljit_emit_op2(C, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_ror_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    if (shift_imm != 0) {
        sljit_emit_op2(C, SLJIT_ROTR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    }
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_rors_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int shift_imm = (opcode >> 7) & 0x1F;
    
    emit_load_reg(C, SLJIT_R0, rm);
    if (shift_imm != 0) {
        sljit_emit_op2(C, SLJIT_ROTR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, shift_imm);
    }
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_lsl_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int rs = (opcode >> 8) & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_lsr_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int rs = (opcode >> 8) & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_asr_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int rs = (opcode >> 8) & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    sljit_emit_op2(C, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_ror_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    int rs = (opcode >> 8) & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rs);
    sljit_emit_op2(C, SLJIT_ROTR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_rrx(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R1, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 1);
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 31);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

/* --------------------------------------------------------------------- */
/*  Additional Data Processing Instructions                              */
/* --------------------------------------------------------------------- */

static int translate_rsb_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, imm_val);
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_rsb_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_adc_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, imm_val);
    sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R3, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_sbc_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, imm_val);
    sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R3, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_rsc_imm(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    uint32_t imm = opcode & 0xFF;
    int rotate = ((opcode >> 8) & 0xF) << 1;
    uint32_t imm_val = (imm >> rotate) | (imm << (32 - rotate));
    
    emit_load_reg(C, SLJIT_R1, rn);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, imm_val);
    sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R1, 0);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R3, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_rsc_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op1(C, SLJIT_MOV_U8, SLJIT_R3, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R2, 0, SLJIT_R1, 0);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R3, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_adc_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R3, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R3, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_sbc_reg(struct sljit_compiler *C, uint32_t opcode)
{
    int rd = (opcode >> 12) & 0xF;
    int rn = (opcode >> 16) & 0xF;
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R1, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R3, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R3, 0, SLJIT_R3, 0, SLJIT_IMM, 1);
    
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R1, 0, SLJIT_R2, 0);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R3, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

#define translate_adcs_imm translate_adc_imm
#define translate_adcs_reg translate_adc_reg
#define translate_sbcs_imm translate_sbc_imm
#define translate_sbcs_reg translate_sbc_reg
#define translate_rscs_imm translate_rsc_imm
#define translate_rscs_reg translate_rsc_reg

/* --------------------------------------------------------------------- */
/*  SWI Instruction                                                      */
/* --------------------------------------------------------------------- */

static void jit_swi_handler(gba_cpu_state_t *cpu, uint32_t swi_num, uint32_t pc)
{
    (void)swi_num;
    
    cpu->reg[14] = pc + 4;
    
    cpu->spsr = cpu->cpsr;
    
    cpu->cpsr = (cpu->cpsr & ~0x3F) | 0x13 | 0x80;
    
    cpu->reg[15] = 0x00000008;
}

static int translate_swi(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    uint32_t swi_num = opcode & 0x00FFFFFF;
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_S0, 0);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, swi_num);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, SLJIT_IMM, pc);
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS3V(P, 32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(jit_swi_handler));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, 0x00000008);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    (void)swi_num;
    return 1;
}

/* --------------------------------------------------------------------- */
/*  Branch Instructions                                                  */
/* --------------------------------------------------------------------- */

static int translate_b(struct sljit_compiler *C, uint32_t opcode, uint32_t pc, uint8_t cond)
{
    int32_t offset = (int32_t)((opcode & 0x00FFFFFF) << 2);
    if (offset & 0x02000000) offset |= 0xFC000000;
    uint32_t target = pc + 8 + offset;
    uint32_t next_pc = pc + 8;
    
    if (target < 0x1000) {
        ESP_LOGW(TAG, "B指令异常目标: PC=0x%08X -> target=0x%08X, offset=%d", 
                 pc, target, offset);
    }
    
    if (cond == ARM_COND_AL) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    } else {
        struct sljit_jump *skip_branch = emit_condition_check_inverse(C, cond);
        
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        
        sljit_set_label(skip_branch, sljit_emit_label(C));
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, next_pc);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    }
    
    return 1;
}

__attribute__((unused))
static int translate_bl(struct sljit_compiler *C, uint32_t opcode, uint32_t pc, uint8_t cond)
{
    int32_t offset = (int32_t)((opcode & 0x00FFFFFF) << 2);
    if (offset & 0x02000000) offset |= 0xFC000000;
    uint32_t target = pc + 8 + offset;
    uint32_t return_addr = pc + 8;
    uint32_t next_pc = pc + 8;
    
    if (target < 0x1000 || (target >= 0x03000000 && target < 0x04000000)) {
        ESP_LOGW(TAG, "BL指令异常目标: PC=0x%08X -> target=0x%08X, offset=%d", 
                 pc, target, offset);
    }
    
    if (cond == ARM_COND_AL) {
        sljit_emit_op1(C, SLJIT_MOV32, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[14]),
                       SLJIT_IMM, return_addr);
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    } else {
        struct sljit_jump *skip_branch = emit_condition_check_inverse(C, cond);
        
        sljit_emit_op1(C, SLJIT_MOV32, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[14]),
                       SLJIT_IMM, return_addr);
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        
        sljit_set_label(skip_branch, sljit_emit_label(C));
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, next_pc);
        sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    }
    
    return 1;
}

static int translate_bx(struct sljit_compiler *C, uint32_t opcode)
{
    int rm = opcode & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cpsr));
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0xFFFFFFDF);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, 
                   SLJIT_R1, 0);
    sljit_emit_op1(C, SLJIT_MOV32, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cpsr),
                   SLJIT_R2, 0);
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFE);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static int translate_blx_reg(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int rm = opcode & 0xF;
    uint32_t return_addr = pc + 8;
    
    sljit_emit_op1(C, SLJIT_MOV32, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[14]),
                   SLJIT_IMM, return_addr);
    
    emit_load_reg(C, SLJIT_R0, rm);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 1);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cpsr));
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 0xFFFFFFDF);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R2, 0, SLJIT_R2, 0, 
                   SLJIT_R1, 0);
    sljit_emit_op1(C, SLJIT_MOV32, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cpsr),
                   SLJIT_R2, 0);
    
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFE);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static int translate_blx_imm(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    int32_t offset = (int32_t)((opcode & 0x00FFFFFF) << 2);
    if (offset & 0x02000000) offset |= 0xFC000000;
    uint32_t target = (pc + 8 + offset) & 0xFFFFFFFC;
    uint32_t return_addr = pc + 8;
    bool thumb = (opcode & 2) != 0;
    
    sljit_emit_op1(C, SLJIT_MOV32, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[14]),
                   SLJIT_IMM, return_addr);
    
    if (thumb) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cpsr));
        sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0x20);
        sljit_emit_op1(C, SLJIT_MOV32, 
                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cpsr),
                       SLJIT_R0, 0);
    }
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target | (thumb ? 1 : 0));
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

/* --------------------------------------------------------------------- */
/*  Main Translation Function                                            */
/* --------------------------------------------------------------------- */

static int translate_instruction(struct sljit_compiler *C, uint32_t opcode, uint32_t pc)
{
    uint8_t cond = (opcode >> 28) & 0xF;
    struct sljit_jump *cond_jump = NULL;
    
    uint32_t bits_27_25 = (opcode >> 25) & 0x7;
    
    bool is_branch = (bits_27_25 == 0x5 || bits_27_25 == 0x6);
    
    if (cond != ARM_COND_AL && !is_branch) {
        cond_jump = emit_condition_check(C, cond);
    }
    
    int result = 0;
    
    switch (bits_27_25) {
        case 0x0:
            if ((opcode & 0x0FC000F0) == 0x00000090) {
                result = translate_mul(C, opcode);
            } else if ((opcode & 0x0FC000F0) == 0x01000090) {
                result = translate_mla(C, opcode);
            } else if ((opcode & 0x0F8000F0) == 0x00800090) {
                result = translate_umull(C, opcode);
            } else if ((opcode & 0x0FB00FF0) == 0x01200010) {
                result = translate_bx(C, opcode);
            } else if ((opcode & 0x0FFFFFF0) == 0x012FFF30) {
                result = translate_blx_reg(C, opcode, pc);
            } else if ((opcode & 0x0FB00FF0) == 0x010000D0) {
                result = translate_ldrsb_imm(C, opcode, pc);
            } else if ((opcode & 0x0FB00FF0) == 0x010000F0) {
                result = translate_ldrsh_imm(C, opcode, pc);
            } else if ((opcode & 0x0FC000F0) == 0x000000B0) {
                result = translate_ldrh_imm(C, opcode, pc);
            } else {
                bool is_imm = (opcode >> 25) & 0x01;
                bool s_bit = (opcode >> 20) & 0x01;
                uint32_t op1 = (opcode >> 21) & 0x0F;
                
                switch (op1) {
                    case 0x0:
                        if (is_imm)
                            result = s_bit ? translate_ands_imm(C, opcode) : translate_and_imm(C, opcode);
                        else
                            result = s_bit ? translate_ands_reg(C, opcode) : translate_and_reg(C, opcode);
                        break;
                    case 0x1:
                        if (is_imm)
                            result = s_bit ? translate_eors_imm(C, opcode) : translate_eor_imm(C, opcode);
                        else
                            result = s_bit ? translate_eors_reg(C, opcode) : translate_eor_reg(C, opcode);
                        break;
                    case 0x2:
                        if (is_imm)
                            result = s_bit ? translate_subs_imm(C, opcode) : translate_sub_imm(C, opcode);
                        else
                            result = s_bit ? translate_subs_reg(C, opcode) : translate_sub_reg(C, opcode);
                        break;
                    case 0x3:
                        if (is_imm)
                            result = s_bit ? translate_rsbs_imm(C, opcode) : translate_rsb_imm(C, opcode);
                        else
                            result = s_bit ? translate_rsbs_reg(C, opcode) : translate_rsb_reg(C, opcode);
                        break;
                    case 0x4:
                        if (is_imm)
                            result = s_bit ? translate_adds_imm(C, opcode) : translate_add_imm(C, opcode);
                        else
                            result = s_bit ? translate_adds_reg(C, opcode) : translate_add_reg(C, opcode);
                        break;
                    case 0x5:
                        if (is_imm)
                            result = s_bit ? translate_adcs_imm(C, opcode) : translate_adc_imm(C, opcode);
                        else
                            result = s_bit ? translate_adcs_reg(C, opcode) : translate_adc_reg(C, opcode);
                        break;
                    case 0x6:
                        if (is_imm)
                            result = s_bit ? translate_sbcs_imm(C, opcode) : translate_sbc_imm(C, opcode);
                        else
                            result = s_bit ? translate_sbcs_reg(C, opcode) : translate_sbc_reg(C, opcode);
                        break;
                    case 0x7:
                        if (is_imm)
                            result = s_bit ? translate_rscs_imm(C, opcode) : translate_rsc_imm(C, opcode);
                        else
                            result = s_bit ? translate_rscs_reg(C, opcode) : translate_rsc_reg(C, opcode);
                        break;
                    case 0x8:
                        if (is_imm)
                            result = translate_tst_imm(C, opcode);
                        else
                            result = translate_tst_reg(C, opcode);
                        break;
                    case 0x9:
                        if (is_imm)
                            result = translate_teq_imm(C, opcode);
                        else
                            result = translate_teq_reg(C, opcode);
                        break;
                    case 0xA:
                        if (is_imm)
                            result = translate_cmp_imm(C, opcode);
                        else
                            result = translate_cmp_reg(C, opcode);
                        break;
                    case 0xB:
                        if (is_imm)
                            result = translate_cmn_imm(C, opcode);
                        else
                            result = translate_cmn_reg(C, opcode);
                        break;
                    case 0xC:
                        if (is_imm)
                            result = s_bit ? translate_orrs_imm(C, opcode) : translate_orr_imm(C, opcode);
                        else
                            result = s_bit ? translate_orrs_reg(C, opcode) : translate_orr_reg(C, opcode);
                        break;
                    case 0xD:
                        if (is_imm)
                            result = s_bit ? translate_movs_imm(C, opcode) : translate_mov_imm(C, opcode);
                        else
                            result = s_bit ? translate_movs_reg(C, opcode) : translate_mov_reg(C, opcode);
                        break;
                    case 0xE:
                        if (is_imm)
                            result = s_bit ? translate_bics_imm(C, opcode) : translate_bic_imm(C, opcode);
                        else
                            result = s_bit ? translate_bics_reg(C, opcode) : translate_bic_reg(C, opcode);
                        break;
                    case 0xF:
                        if (is_imm)
                            result = s_bit ? translate_mvns_imm(C, opcode) : translate_mvn_imm(C, opcode);
                        else
                            result = s_bit ? translate_mvns_reg(C, opcode) : translate_mvn_reg(C, opcode);
                        break;
                    default:
                        result = -1;
                        break;
                }
            }
            break;
            
        case 0x1:
            if ((opcode & 0x0FC000F0) == 0x01000090) {
                result = translate_mla(C, opcode);
            } else if ((opcode & 0x0F8000F0) == 0x00800090) {
                uint32_t op = (opcode >> 21) & 0x7;
                switch (op) {
                    case 0x0: result = translate_umull(C, opcode); break;
                    case 0x2: result = translate_umlal(C, opcode); break;
                    case 0x4: result = translate_smull(C, opcode); break;
                    case 0x6: result = translate_smlal(C, opcode); break;
                    default: result = translate_umull(C, opcode); break;
                }
            } else if ((opcode & 0x0FB00FF0) == 0x01200010) {
                result = translate_bx(C, opcode);
            } else {
                bool s_bit = (opcode >> 20) & 0x01;
                bool is_imm = (opcode >> 25) & 0x01;
                if (is_imm) {
                    result = s_bit ? translate_movs_imm(C, opcode) : translate_mov_imm(C, opcode);
                } else {
                    result = s_bit ? translate_movs_reg(C, opcode) : translate_mov_reg(C, opcode);
                }
            }
            break;
            
        case 0x2:
        case 0x3:
            {
                bool is_load = (opcode >> 20) & 0x01;
                bool is_byte = (opcode >> 22) & 0x01;
                bool is_imm = !((opcode >> 25) & 0x01);
                
                if (is_load) {
                    if (is_byte) {
                        result = translate_ldrb_imm(C, opcode, pc);
                    } else {
                        if (is_imm) {
                            result = translate_ldr_imm(C, opcode, pc);
                        } else {
                            result = translate_ldr_reg(C, opcode);
                        }
                    }
                } else {
                    if (is_byte) {
                        result = translate_strb_imm(C, opcode);
                    } else {
                        if (is_imm) {
                            result = translate_str_imm(C, opcode);
                        } else {
                            result = translate_str_reg(C, opcode);
                        }
                    }
                }
            }
            break;
            
        case 0x4:
            {
                bool is_load = (opcode >> 20) & 0x01;
                if (is_load) {
                    result = translate_ldm(C, opcode);
                } else {
                    result = translate_stm(C, opcode);
                }
            }
            break;
            
        case 0x5:
            if (cond == 0xF && (opcode & 0x01000000)) {
                result = translate_blx_imm(C, opcode, pc);
            } else if (opcode & 0x01000000) {
                result = translate_bl(C, opcode, pc, cond);
            } else {
                result = translate_b(C, opcode, pc, cond);
            }
            break;
            
        case 0x6:
            result = translate_bl(C, opcode, pc, cond);
            break;
            
        case 0x7:
            if (opcode & 0x10) {
                result = translate_swi(C, opcode, pc);
            } else {
                result = -1;
            }
            break;
            
        default:
            result = -1;
            break;
    }
    
    if (cond_jump != NULL) {
        sljit_set_label(cond_jump, sljit_emit_label(C));
    }
    
    return result;
}

/* --------------------------------------------------------------------- */
/*  Thumb Instruction Translation                                        */
/* --------------------------------------------------------------------- */

static int translate_thumb_mov_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    uint32_t imm = opcode & 0xFF;
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_add_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rn = (opcode >> 3) & 0x7;
    int rm = (opcode >> 6) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_sub_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rn = (opcode >> 3) & 0x7;
    int rm = (opcode >> 6) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_add_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t imm = (opcode >> 6) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_sub_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t imm = (opcode >> 6) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_add_imm8(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = (opcode >> 8) & 0x7;
    uint32_t imm = opcode & 0xFF;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, rdn, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_sub_imm8(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = (opcode >> 8) & 0x7;
    uint32_t imm = opcode & 0xFF;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, rdn, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_and_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_eor_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_lsl_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_SHL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_lsr_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_LSHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_asr_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_ASHR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_adc_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 1);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_sbc_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R2, 0, 
                   SLJIT_MEM1(SLJIT_S0), C_FLAG_OFFSET);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R2, 0, SLJIT_R2, 0, SLJIT_IMM, 1);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_tst_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_cmp_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_cmn_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_orr_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_OR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_mul_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_MUL32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_bic_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rdn = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rdn);
    emit_load_reg(C, SLJIT_R1, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, 0xFFFFFFFF);
    sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R1, 0);
    emit_store_reg(C, rdn, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_mvn_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rm = (opcode >> 3) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rm);
    sljit_emit_op2(C, SLJIT_XOR32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFF);
    emit_store_reg(C, rd, SLJIT_R0);
    emit_update_flags_nz_simple(C, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_ldr_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t offset = (opcode & 0x7C0) >> 4;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_str_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t offset = (opcode & 0x7C0) >> 4;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
    
    return 0;
}

static int translate_thumb_str_reg(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = opcode & 0x7;
    int rn = (opcode >> 3) & 0x7;
    int rm = (opcode >> 6) & 0x7;
    
    emit_load_reg(C, SLJIT_R0, rn);
    emit_load_reg(C, SLJIT_R2, rm);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_R2, 0);
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_ldrb_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t offset = (opcode & 0x7C0) >> 6;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read8));
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

static int translate_thumb_strb_imm(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    int rn = (opcode >> 3) & 0x7;
    uint32_t offset = (opcode & 0x7C0) >> 6;
    
    emit_load_reg(C, SLJIT_R0, rn);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write8));
    
    return 0;
}

static int translate_thumb_ldr_pc(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    int rd = (opcode >> 8) & 0x7;
    uint32_t offset = (opcode & 0xFF) << 2;
    uint32_t addr = (pc + 4 + offset) & ~3;
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, addr);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_ldr_sp(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    uint32_t offset = (opcode & 0xFF) << 2;
    
    emit_load_reg(C, SLJIT_R0, 13);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
    emit_store_reg(C, rd, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_str_sp(struct sljit_compiler *C, uint16_t opcode)
{
    int rd = (opcode >> 8) & 0x7;
    uint32_t offset = (opcode & 0xFF) << 2;
    
    emit_load_reg(C, SLJIT_R0, 13);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, offset);
    emit_load_reg(C, SLJIT_R1, rd);
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_add_sp_imm(struct sljit_compiler *C, uint16_t opcode)
{
    uint32_t imm = (opcode & 0x7F) << 2;
    
    emit_load_reg(C, SLJIT_R0, 13);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, 13, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_sub_sp_imm(struct sljit_compiler *C, uint16_t opcode)
{
    uint32_t imm = (opcode & 0x7F) << 2;
    
    emit_load_reg(C, SLJIT_R0, 13);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, imm);
    emit_store_reg(C, 13, SLJIT_R0);
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_push(struct sljit_compiler *C, uint16_t opcode)
{
    uint8_t reg_list = opcode & 0xFF;
    bool push_lr = (opcode >> 8) & 1;
    int num_regs = count_bits(reg_list) + (push_lr ? 1 : 0);
    
    emit_load_reg(C, SLJIT_R0, 13);
    sljit_emit_op2(C, SLJIT_SUB32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, num_regs * 4);
    emit_store_reg(C, 13, SLJIT_R0);
    
    int offset = 0;
    for (int i = 0; i < 8; i++) {
        if ((reg_list >> i) & 1) {
            sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, offset);
            emit_load_reg(C, SLJIT_R2, i);
            sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                             SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
            offset += 4;
        }
    }
    
    if (push_lr) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
        sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, offset);
        emit_load_reg(C, SLJIT_R2, 14);
        sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(32, 32),
                         SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_write32));
    }
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_pop(struct sljit_compiler *C, uint16_t opcode)
{
    uint8_t reg_list = opcode & 0xFF;
    bool pop_pc = (opcode >> 8) & 1;
    int num_regs = count_bits(reg_list);
    
    emit_load_reg(C, SLJIT_R0, 13);
    
    int offset = 0;
    for (int i = 0; i < 8; i++) {
        if ((reg_list >> i) & 1) {
            sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
            sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, offset);
            sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                             SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
            emit_store_reg(C, i, SLJIT_R0);
            offset += 4;
        }
    }
    
    if (pop_pc) {
        sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_R0, 0);
        sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R1, 0, SLJIT_R1, 0, SLJIT_IMM, offset);
        sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS1(W, 32),
                         SLJIT_IMM, SLJIT_FUNC_ADDR(gba_mem_read32));
        sljit_emit_op2(C, SLJIT_AND32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, 0xFFFFFFFE);
        emit_store_reg(C, 15, SLJIT_R0);
    }
    
    emit_load_reg(C, SLJIT_R0, 13);
    sljit_emit_op2(C, SLJIT_ADD32, SLJIT_R0, 0, SLJIT_R0, 0, SLJIT_IMM, (num_regs + (pop_pc ? 1 : 0)) * 4);
    emit_store_reg(C, 13, SLJIT_R0);
    
    return pop_pc ? 1 : 0;
}

static int translate_thumb_b(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    int32_t offset = (int8_t)(opcode & 0xFF);
    offset <<= 1;
    uint32_t target = pc + 4 + offset;
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
    emit_store_reg(C, 15, SLJIT_R0);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static int translate_thumb_b_cond(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    int cond = (opcode >> 8) & 0xF;
    int32_t offset = (int8_t)(opcode & 0xFF);
    offset <<= 1;
    uint32_t target = pc + 4 + offset;
    
    struct sljit_jump *cond_jump = emit_condition_check(C, cond);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
    emit_store_reg(C, 15, SLJIT_R0);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    if (cond_jump != NULL) {
        sljit_set_label(cond_jump, sljit_emit_label(C));
    }
    
    return 0;
}

__attribute__((unused))
static int translate_thumb_bl(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    int32_t offset = (opcode & 0x7FF);
    if (offset & 0x400) {
        offset |= 0xFFFFF800;
    }
    offset <<= 1;
    uint32_t target = pc + 4 + offset;
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, (pc + 4) | 1);
    emit_store_reg(C, 14, SLJIT_R0);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target);
    emit_store_reg(C, 15, SLJIT_R0);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static int translate_thumb_swi(struct sljit_compiler *C, uint16_t opcode)
{
    uint8_t swi_num = opcode & 0xFF;
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_S0, 0);
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R1, 0, SLJIT_IMM, swi_num);
    
    sljit_emit_icall(C, SLJIT_CALL, SLJIT_ARGS2V(P, 32),
                     SLJIT_IMM, SLJIT_FUNC_ADDR(jit_swi_handler));
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, 0x00000008);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static uint32_t thumb_bl_offset = 0;

static int translate_thumb_bl_prefix(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    int32_t offset = (opcode & 0x7FF);
    if (offset & 0x400) {
        offset |= 0xFFFFF800;
    }
    offset <<= 12;
    
    thumb_bl_offset = offset;
    
    return 0;
}

static int translate_thumb_bl_suffix(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    int32_t offset = thumb_bl_offset | ((opcode & 0x7FF) << 1);
    
    uint32_t target = pc + 2 + offset;
    uint32_t return_addr = (pc + 2) | 1;
    
    sljit_emit_op1(C, SLJIT_MOV32, 
                   SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, reg[14]),
                   SLJIT_IMM, return_addr);
    
    sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, target | 1);
    emit_store_reg(C, 15, SLJIT_R0);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static int translate_thumb_bx(struct sljit_compiler *C, uint16_t opcode)
{
    int rm = (opcode >> 3) & 0xF;
    
    emit_load_reg(C, SLJIT_R0, rm);
    emit_store_reg(C, 15, SLJIT_R0);
    sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
    
    return 1;
}

static int translate_thumb_instruction(struct sljit_compiler *C, uint16_t opcode, uint32_t pc)
{
    uint16_t op = (opcode >> 11) & 0x1F;
    int result = 0;
    
    switch (op) {
        case 0x00:
        case 0x01:
        case 0x02:
            {
                uint16_t op2 = (opcode >> 9) & 0x7;
                switch (op2) {
                    case 0x0: result = translate_thumb_lsl_reg(C, opcode); break;
                    case 0x1: result = translate_thumb_lsr_reg(C, opcode); break;
                    case 0x2: result = translate_thumb_asr_reg(C, opcode); break;
                    case 0x3: result = translate_thumb_add_reg(C, opcode); break;
                    case 0x4: result = translate_thumb_sub_reg(C, opcode); break;
                    case 0x5: result = translate_thumb_add_imm(C, opcode); break;
                    case 0x6: result = translate_thumb_sub_imm(C, opcode); break;
                    default: result = -1; break;
                }
            }
            break;
        case 0x03:
            result = translate_thumb_mov_imm(C, opcode);
            break;
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            result = translate_thumb_cmp_reg(C, opcode);
            break;
        case 0x08:
        case 0x09:
            result = translate_thumb_add_imm8(C, opcode);
            break;
        case 0x0A:
        case 0x0B:
            result = translate_thumb_sub_imm8(C, opcode);
            break;
        case 0x0C:
        case 0x0D:
        case 0x0E:
            {
                uint16_t op2 = (opcode >> 6) & 0xF;
                switch (op2) {
                    case 0x0: result = translate_thumb_and_reg(C, opcode); break;
                    case 0x1: result = translate_thumb_eor_reg(C, opcode); break;
                    case 0x2: result = translate_thumb_lsl_reg(C, opcode); break;
                    case 0x3: result = translate_thumb_lsr_reg(C, opcode); break;
                    case 0x4: result = translate_thumb_asr_reg(C, opcode); break;
                    case 0x5: result = translate_thumb_adc_reg(C, opcode); break;
                    case 0x6: result = translate_thumb_sbc_reg(C, opcode); break;
                    case 0x7: result = translate_thumb_tst_reg(C, opcode); break;
                    case 0x8: result = translate_thumb_tst_reg(C, opcode); break;
                    case 0x9: result = translate_thumb_cmp_reg(C, opcode); break;
                    case 0xA: result = translate_thumb_cmn_reg(C, opcode); break;
                    case 0xB: result = translate_thumb_orr_reg(C, opcode); break;
                    case 0xC: result = translate_thumb_mul_reg(C, opcode); break;
                    case 0xD: result = translate_thumb_bic_reg(C, opcode); break;
                    case 0xE: result = translate_thumb_mvn_reg(C, opcode); break;
                    case 0xF: result = translate_thumb_mvn_reg(C, opcode); break;
                    default: result = -1; break;
                }
            }
            break;
        case 0x0F:
            {
                uint16_t op2 = (opcode >> 8) & 0x7;
                switch (op2) {
                    case 0x0: result = translate_thumb_add_reg(C, opcode); break;
                    case 0x1: result = translate_thumb_cmp_reg(C, opcode); break;
                    case 0x2: result = translate_thumb_mov_imm(C, opcode); break;
                    case 0x3: result = translate_thumb_bx(C, opcode); break;
                    case 0x4:
                    case 0x5:
                    case 0x6:
                    case 0x7:
                        result = translate_thumb_ldr_pc(C, opcode, pc);
                        break;
                    default: result = -1; break;
                }
            }
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13:
        case 0x14:
            result = translate_thumb_ldr_pc(C, opcode, pc);
            break;
        case 0x15:
        case 0x16:
            result = translate_thumb_str_reg(C, opcode);
            break;
        case 0x17:
        case 0x18:
            result = translate_thumb_strb_imm(C, opcode);
            break;
        case 0x19:
        case 0x1A:
            result = translate_thumb_ldr_imm(C, opcode);
            break;
        case 0x1B:
            result = translate_thumb_ldrh_imm(C, opcode);
            break;
        case 0x1C:
            {
                uint8_t cond = (opcode >> 8) & 0x0F;
                if (cond == 0x0F) {
                    result = translate_thumb_swi(C, opcode);
                } else {
                    result = translate_thumb_b_cond(C, opcode, pc);
                }
            }
            break;
        case 0x1D:
            result = translate_thumb_b(C, opcode, pc);
            break;
        case 0x1E:
            result = translate_thumb_bl_prefix(C, opcode, pc);
            break;
        case 0x1F:
            result = translate_thumb_bl_suffix(C, opcode, pc);
            break;
        default:
            result = -1;
            break;
    }
    
    return result;
}

static bool is_thumb_block_terminator(uint16_t opcode)
{
    uint16_t op = (opcode >> 11) & 0x1F;
    
    if (op == 0x0F && ((opcode >> 8) & 0x7) == 0x3) {
        return true;
    }
    
    if (op >= 0x1C && op <= 0x1F) {
        return true;
    }
    
    return false;
}

static int get_arm_instruction_cycles(uint32_t opcode)
{
    uint32_t bits_27_25 = (opcode >> 25) & 0x7;
    
    switch (bits_27_25) {
        case 0x0:
            if ((opcode & 0x0FC000F0) == 0x00000090 ||
                (opcode & 0x0FC000F0) == 0x01000090) {
                return 3;
            }
            if ((opcode & 0x0F8000F0) == 0x00800090) {
                return 4;
            }
            return 1;
            
        case 0x1:
            return 1;
            
        case 0x2:
        case 0x3:
            if ((opcode >> 20) & 0x01) {
                return 2;
            }
            if ((opcode >> 24) & 0x01) {
                return 2;
            }
            return 1;
            
        case 0x4:
            return 2;
            
        case 0x5:
            return 2;
            
        case 0x6:
            return 2;
            
        case 0x7:
            return 2;
            
        default:
            return 1;
    }
}

static int get_thumb_instruction_cycles(uint16_t opcode)
{
    uint16_t op = (opcode >> 11) & 0x1F;
    
    switch (op) {
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
            return 2;
            
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B:
        case 0x0C:
        case 0x0D:
        case 0x0E:
            return 2;
            
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F:
            return 2;
            
        default:
            return 1;
    }
}

__attribute__((unused)) static void emit_cycle_check(struct sljit_compiler *C)
{
    struct sljit_jump *out_of_cycles = sljit_emit_cmp(C, SLJIT_32 | SLJIT_LESS_EQUAL,
                                                       SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cycles),
                                                       SLJIT_IMM, 0);
    
    sljit_set_label(out_of_cycles, sljit_emit_label(C));
}

bool jit_translate_block(struct sljit_compiler *C, uint32_t pc, bool is_thumb, uint32_t *end_pc)
{
    uint32_t current_pc = pc;
    uint32_t max_pc = pc + g_jit_config.max_block_instructions * (is_thumb ? 2 : 4);
    int instruction_count = 0;
    bool l1_only = true;
    bool has_error = false;
    bool has_return = false;
    
    // 临时注释 - 调试日志
    // ESP_LOGD(TAG, "开始翻译块: PC=0x%08X, thumb=%d", pc, is_thumb);
    
    if (is_thumb) {
        while (current_pc < max_pc && instruction_count < g_jit_config.max_block_instructions) {
            uint16_t opcode = gba_mem_read16(current_pc);
            
            if (!is_l1_thumb_instruction(opcode)) {
                // 临时注释 - 调试日志
                // ESP_LOGD(TAG, "Non-L1 Thumb instruction at PC=0x%08X: 0x%04X, falling back", current_pc, opcode);
                l1_only = false;
                break;
            }
            
            int result = translate_thumb_instruction(C, opcode, current_pc);
            
            if (result < 0) {
                ESP_LOGW(TAG, "Unknown Thumb instruction at PC=0x%08X: 0x%04X", current_pc, opcode);
                has_error = true;
                break;
            }
            
            int cycles = get_thumb_instruction_cycles(opcode);
            sljit_emit_op2(C, SLJIT_SUB32, 
                           SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cycles),
                           SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cycles),
                           SLJIT_IMM, cycles);
            
            instruction_count++;
            current_pc += 2;
            
            if (result > 0) {
                has_return = true;
                break;
            }
            if (is_thumb_block_terminator(opcode)) {
                break;
            }
        }
        
        if (instruction_count > 0 && !has_error && !has_return) {
            sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, current_pc);
            sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        }
    } else {
        while (current_pc < max_pc && instruction_count < g_jit_config.max_block_instructions) {
            uint32_t opcode = gba_mem_read32(current_pc);
            
            if (!is_l1_instruction(opcode)) {
                // 临时注释 - 调试日志
                // ESP_LOGD(TAG, "Non-L1 instruction at PC=0x%08X: 0x%08X, falling back", current_pc, opcode);
                l1_only = false;
                break;
            }
            
            int result = translate_instruction(C, opcode, current_pc);
            
            if (result < 0) {
                ESP_LOGW(TAG, "Unknown instruction at PC=0x%08X: 0x%08X", current_pc, opcode);
                has_error = true;
                break;
            }
            
            int cycles = get_arm_instruction_cycles(opcode);
            sljit_emit_op2(C, SLJIT_SUB32, 
                           SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cycles),
                           SLJIT_MEM1(SLJIT_S0), offsetof(gba_cpu_state_t, cycles),
                           SLJIT_IMM, cycles);
            
            instruction_count++;
            current_pc += 4;
            
            if (result > 0) {
                has_return = true;
                break;
            }
            if (is_block_terminator(opcode)) {
                break;
            }
        }
        
        if (instruction_count > 0 && !has_error && !has_return) {
            sljit_emit_op1(C, SLJIT_MOV32, SLJIT_R0, 0, SLJIT_IMM, current_pc);
            sljit_emit_return(C, SLJIT_MOV, SLJIT_R0, 0);
        }
    }
    
    *end_pc = current_pc;
    
    if (instruction_count == 0 || has_error) {
        *end_pc = pc;
        return false;
    }
    
    // 临时注释 - 调试日志
    // ESP_LOGD(TAG, "翻译完成: PC=0x%08X -> 0x%08X, 指令数=%d, l1_only=%d", 
    //          pc, current_pc, instruction_count, l1_only);
    
    return l1_only;
}
