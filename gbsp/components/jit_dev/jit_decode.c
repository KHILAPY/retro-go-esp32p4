/*
 * ARM Instruction Decoder for GBA JIT
 * 
 * Decodes ARM7TDMI instructions and determines translation level.
 */

#include "jit_core.h"
#include <string.h>

/* --------------------------------------------------------------------- */
/*  ARM Condition Codes                                                  */
/* --------------------------------------------------------------------- */

static const char *cond_names[] = {
    "EQ", "NE", "CS", "CC", "MI", "PL", "VS", "VC",
    "HI", "LS", "GE", "LT", "GT", "LE", "AL", "NV"
};

/* --------------------------------------------------------------------- */
/*  Immediate Value Expansion                                            */
/* --------------------------------------------------------------------- */

static inline uint32_t arm_expand_imm(uint8_t imm, uint8_t rotate)
{
    if (rotate == 0) {
        return imm;
    }
    return (imm >> (rotate * 2)) | (imm << (32 - rotate * 2));
}

/* --------------------------------------------------------------------- */
/*  Instruction Type Detection                                           */
/* --------------------------------------------------------------------- */

static insn_type_t detect_insn_type(uint32_t opcode)
{
    uint32_t type = (opcode >> 25) & 0x07;
    
    switch (type) {
        case 0x00:
        case 0x01:
            if ((opcode & 0x0FFFFFF0) == 0x012FFF10) {
                return INSN_TYPE_BRANCH;
            }
            if ((opcode & 0x0FC000F0) == 0x000000B0) {
                return INSN_TYPE_LOAD_STORE;
            }
            if ((opcode & 0x0FC000F0) == 0x00000090) {
                return INSN_TYPE_DATA_PROC;
            }
            return INSN_TYPE_DATA_PROC;
            
        case 0x02:
        case 0x03:
            return INSN_TYPE_LOAD_STORE;
            
        case 0x04:
            return INSN_TYPE_BLOCK_TRANSFER;
            
        case 0x05:
            return INSN_TYPE_BRANCH;
            
        case 0x06:
            return INSN_TYPE_COPROC;
            
        case 0x07:
            if (opcode & 0x10) {
                return INSN_TYPE_SWI;
            }
            return INSN_TYPE_COPROC;
            
        default:
            return INSN_TYPE_UNKNOWN;
    }
}

/* --------------------------------------------------------------------- */
/*  L1 Instruction Detection                                             */
/* --------------------------------------------------------------------- */

/*
 * JIT指令验证状态 (2026-02-23)
 * 
 * ✅ 已验证正常：
 *    - 数据处理指令 (type=0x00/0x01):
 *      ✅ ADD, SUB, MOV, AND, ORR, CMP
 * 
 * 🔶 已启用待验证：
 *    - 数据处理指令: EOR, BIC, MVN, TST, ADC, SBC, TEQ, CMN, RSB, RSC
 * 
 * ❌ 已验证有问题：
 *    - 分支指令 (type=0x05): B, BL - 导致PC跳转错误
 *    - 加载/存储指令 (type=0x02/0x03): LDR, STR - 导致Load access fault崩溃
 * 
 * ⏳ 待验证：
 *    - 块传输指令 (type=0x04): LDM, STM
 */

bool is_l1_instruction(uint32_t opcode)
{
    uint32_t type = (opcode >> 25) & 0x07;
    
    /* ✅ 数据处理指令 - 已验证正常 */
    if (type == 0x00 || type == 0x01) {
        uint32_t opcode_field = (opcode >> 21) & 0x0F;
        
        /* BX 指令 - 排除 */
        if ((opcode & 0x0FFFFFF0) == 0x012FFF10) {
            return false;
        }
        
        /* MUL 指令 - 排除 */
        if ((opcode & 0x0FC000F0) == 0x00000090) {
            return false;
        }
        
        /* MLA 指令 - 排除 */
        if ((opcode & 0x0FC000F0) == 0x01000090) {
            return false;
        }
        
        /* UMULL/SMULL 指令 - 排除 */
        if ((opcode & 0x0F8000F0) == 0x00800090) {
            return false;
        }
        
        switch (opcode_field) {
            case 0x00:  /* ✅ AND - 已验证正常 */
            case 0x01:  /* 🔶 EOR - 已启用待验证 */
            case 0x02:  /* ✅ SUB - 已验证正常 */
            case 0x03:  /* 🔶 RSB - 已启用待验证 */
            case 0x04:  /* ✅ ADD - 已验证正常 */
            case 0x05:  /* 🔶 ADC - 已启用待验证 */
            case 0x06:  /* 🔶 SBC - 已启用待验证 */
            case 0x07:  /* 🔶 RSC - 已启用待验证 */
            case 0x08:  /* 🔶 TST - 已启用待验证 */
            case 0x09:  /* 🔶 TEQ - 已启用待验证 */
            case 0x0A:  /* ✅ CMP - 已验证正常 */
            case 0x0B:  /* 🔶 CMN - 已启用待验证 */
            case 0x0C:  /* ✅ ORR - 已验证正常 */
            case 0x0D:  /* ✅ MOV - 已验证正常 */
            case 0x0E:  /* 🔶 BIC - 已启用待验证 */
            case 0x0F:  /* 🔶 MVN - 已启用待验证 */
                return true;
            default:
                return false;
        }
    }
    
    /* ✅ 加载/存储指令 - 标志同步修复后启用 */
    if (type == 0x02 || type == 0x03) {
        return true;
    }
    
    /* ⏳ 块传输指令 - 待验证 */
    if (type == 0x04) {
        return false;
    }
    
    /* ✅ 分支指令 - 已启用，标志同步修复后可用 */
    if (type == 0x05) {
        return true;
    }
    
    /* 协处理器指令 - 不支持 */
    if (type == 0x07) {
        return false;
    }
    
    return false;
}

bool is_l1_thumb_instruction(uint16_t opcode)
{
    uint16_t op = (opcode >> 11) & 0x1F;
    
    switch (op) {
        case 0x00:  /* LSL/LSR/ASR/ADD/SUB 寄存器 - ⏳ 待验证 */
        case 0x01:  /* LSL/LSR/ASR/ADD/SUB 寄存器 - ⏳ 待验证 */
        case 0x02:  /* LSL/LSR/ASR/ADD/SUB 寄存器 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x03:  /* MOV/CMP/ADD 立即数 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x04:  /* ALU操作 - ⏳ 待验证 */
        case 0x05:  /* ALU操作 - ⏳ 待验证 */
        case 0x06:  /* ALU操作 - ⏳ 待验证 */
        case 0x07:  /* ALU操作 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x08:  /* ADD 立即数(8位) - ⏳ 待验证 */
        case 0x09:  /* ADD 立即数(8位) - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x0A:  /* SUB 立即数(8位) - ⏳ 待验证 */
        case 0x0B:  /* SUB 立即数(8位) - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x0C:  /* ALU操作(AND/EOR/ADC等) - ⏳ 待验证 */
        case 0x0D:  /* ALU操作(AND/EOR/ADC等) - ⏳ 待验证 */
        case 0x0E:  /* ALU操作(AND/EOR/ADC等) - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x0F:  /* ❌ BX/CMP/MOV/LDR PC - 已验证有问题: BX导致PC跳转错误 */
            return false;  /* ❌ BX指令导致ROM卡死 */
        case 0x10:  /* LDR PC相对 - ⏳ 待验证 */
        case 0x11:  /* LDR PC相对 - ⏳ 待验证 */
        case 0x12:  /* LDR PC相对 - ⏳ 待验证 */
        case 0x13:  /* LDR PC相对 - ⏳ 待验证 */
        case 0x14:  /* LDR PC相对 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x15:  /* STR 寄存器偏移 - ⏳ 待验证 */
        case 0x16:  /* STR 寄存器偏移 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x17:  /* STRB 立即数 - ⏳ 待验证 */
        case 0x18:  /* STRB 立即数 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x19:  /* LDR 立即数 - ⏳ 待验证 */
        case 0x1A:  /* LDR 立即数 - ⏳ 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x1B:  /* ❌ LDRH 立即数 - 待验证 */
            return false;  /* ❌ Thumb指令全部禁用 */
        case 0x1C:  /* ❌ 条件分支/SWI - 已验证有问题: 分支导致PC跳转错误 */
            return false;  /* ❌ 条件分支导致ROM卡死 */
        case 0x1D:  /* ❌ 无条件分支 B - 已验证有问题: PC跳转错误 */
            return false;  /* ❌ B指令导致ROM卡死 */
        case 0x1E:  /* ❌ 长分支前缀 BL前缀 - 已验证有问题: PC跳转错误 */
            return false;  /* ❌ BL前缀导致ROM卡死 */
        case 0x1F:  /* ❌ 长分支后缀 BL后缀 - 已验证有问题: PC跳转错误 */
            return false;  /* ❌ BL后缀导致ROM卡死 */
        default:
            return false;
    }
}

/* --------------------------------------------------------------------- */
/*  Translation Level Determination                                      */
/* --------------------------------------------------------------------- */

trans_level_t jit_get_translation_level(uint32_t opcode, bool is_thumb)
{
    if (is_thumb) {
        if (is_l1_thumb_instruction((uint16_t)opcode)) {
            return TRANS_LEVEL_THUMB;
        }
        return TRANS_LEVEL_NONE;
    }
    
    if (is_l1_instruction(opcode)) {
        return TRANS_LEVEL_L1;
    }
    
    uint8_t cond = opcode >> 28;
    if (cond == 0x0E) {
        uint32_t type = (opcode >> 25) & 0x07;
        
        if (type == 0x00 || type == 0x01) {
            uint32_t s_bit = (opcode >> 20) & 0x01;
            if (s_bit) {
                return TRANS_LEVEL_L2;
            }
        }
        
        if (type == 0x02 || type == 0x03) {
            return TRANS_LEVEL_L2;
        }
    }
    
    return TRANS_LEVEL_NONE;
}

/* --------------------------------------------------------------------- */
/*  Instruction Decoding                                                 */
/* --------------------------------------------------------------------- */

void decode_arm_instruction(uint32_t opcode, arm_insn_t *insn)
{
    memset(insn, 0, sizeof(arm_insn_t));
    
    insn->cond = opcode >> 28;
    insn->type = detect_insn_type(opcode);
    
    switch (insn->type) {
        case INSN_TYPE_DATA_PROC:
            insn->is_imm = (opcode >> 25) & 0x01;
            insn->opcode = (opcode >> 21) & 0x0F;
            insn->set_flags = (opcode >> 20) & 0x01;
            insn->rn = (opcode >> 16) & 0x0F;
            insn->rd = (opcode >> 12) & 0x0F;
            
            if (insn->is_imm) {
                uint8_t imm = opcode & 0xFF;
                uint8_t rotate = (opcode >> 8) & 0x0F;
                insn->imm = arm_expand_imm(imm, rotate);
            } else {
                insn->rm = opcode & 0x0F;
                insn->shift_type = (opcode >> 5) & 0x03;
                insn->shift_imm = (opcode >> 7) & 0x1F;
            }
            break;
            
        case INSN_TYPE_LOAD_STORE:
            insn->is_pre_index = (opcode >> 24) & 0x01;
            insn->is_writeback = (opcode >> 21) & 0x01;
            insn->is_load = (opcode >> 20) & 0x01;
            insn->rn = (opcode >> 16) & 0x0F;
            insn->rd = (opcode >> 12) & 0x0F;
            
            if ((opcode & 0x0FC000F0) == 0x000000B0) {
                insn->is_imm = !((opcode >> 22) & 0x01);
                if (insn->is_imm) {
                    uint8_t imm_hi = (opcode >> 8) & 0x0F;
                    uint8_t imm_lo = opcode & 0x0F;
                    insn->offset = (imm_hi << 4) | imm_lo;
                    if (!(opcode & (1 << 23))) {
                        insn->offset = -insn->offset;
                    }
                } else {
                    insn->rm = opcode & 0x0F;
                }
            } else {
                insn->is_imm = !((opcode >> 25) & 0x01);
                if (insn->is_imm) {
                    insn->offset = opcode & 0xFFF;
                    if (!(opcode & (1 << 23))) {
                        insn->offset = -insn->offset;
                    }
                } else {
                    insn->rm = opcode & 0x0F;
                }
            }
            break;
            
        case INSN_TYPE_BRANCH:
            insn->is_link = (opcode >> 24) & 0x01;
            insn->offset = opcode & 0x00FFFFFF;
            if (insn->offset & 0x00800000) {
                insn->offset |= 0xFF000000;
            }
            insn->offset <<= 2;
            break;
            
        case INSN_TYPE_BLOCK_TRANSFER:
            insn->is_load = (opcode >> 20) & 0x01;
            insn->is_writeback = (opcode >> 21) & 0x01;
            insn->is_pre_index = (opcode >> 24) & 0x01;
            insn->rn = (opcode >> 16) & 0x0F;
            insn->imm = opcode & 0xFFFF;
            break;
            
        default:
            break;
    }
}

/* --------------------------------------------------------------------- */
/*  Block Terminator Detection                                           */
/* --------------------------------------------------------------------- */

bool is_block_terminator(uint32_t opcode)
{
    uint32_t type = (opcode >> 25) & 0x07;
    
    if (type == 0x05) {
        return true;
    }
    
    if (type == 0x07 && (opcode & 0x10)) {
        return true;
    }
    
    if (type == 0x04) {
        bool is_load = (opcode >> 20) & 0x01;
        uint16_t reg_list = opcode & 0xFFFF;
        if (is_load && (reg_list & 0x8000)) {
            return true;
        }
        return false;
    }
    
    if (type == 0x00 || type == 0x01) {
        uint32_t rn = (opcode >> 16) & 0x0F;
        uint32_t rd = (opcode >> 12) & 0x0F;
        uint32_t opcode_field = (opcode >> 21) & 0x0F;
        
        if (rd == 15 || rn == 15) {
            if (opcode_field >= 0x08 && opcode_field <= 0x0D) {
                return true;
            }
        }
        
        if ((opcode & 0x0FFFFFF0) == 0x012FFF10) {
            return true;
        }
    }
    
    return false;
}

/* --------------------------------------------------------------------- */
/*  Debug Functions                                                      */
/* --------------------------------------------------------------------- */

const char *get_cond_name(uint8_t cond)
{
    if (cond < 16) {
        return cond_names[cond];
    }
    return "??";
}

const char *get_opcode_name(uint8_t opcode)
{
    static const char *opcode_names[] = {
        "AND", "EOR", "SUB", "RSB", "ADD", "ADC", "SBC", "RSC",
        "TST", "TEQ", "CMP", "CMN", "ORR", "MOV", "BIC", "MVN"
    };
    if (opcode < 16) {
        return opcode_names[opcode];
    }
    return "???";
}
