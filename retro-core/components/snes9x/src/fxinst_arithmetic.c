/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

#define FX_ADD(reg) \
    { \
        int32_t s = SUSEX16(SREG) + SUSEX16(GSU.avReg[reg]); \
        GSU.vCarry = s >= 0x10000; \
        GSU.vOverflow = ~(SREG ^ GSU.avReg[reg]) & (GSU.avReg[reg] ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_add_r0 (void)  { FX_ADD(0); }
void fx_add_r1 (void)  { FX_ADD(1); }
void fx_add_r2 (void)  { FX_ADD(2); }
void fx_add_r3 (void)  { FX_ADD(3); }
void fx_add_r4 (void)  { FX_ADD(4); }
void fx_add_r5 (void)  { FX_ADD(5); }
void fx_add_r6 (void)  { FX_ADD(6); }
void fx_add_r7 (void)  { FX_ADD(7); }
void fx_add_r8 (void)  { FX_ADD(8); }
void fx_add_r9 (void)  { FX_ADD(9); }
void fx_add_r10 (void) { FX_ADD(10); }
void fx_add_r11 (void) { FX_ADD(11); }
void fx_add_r12 (void) { FX_ADD(12); }
void fx_add_r13 (void) { FX_ADD(13); }
void fx_add_r14 (void) { FX_ADD(14); }
void fx_add_r15 (void) { FX_ADD(15); }

#define FX_ADD_I(imm) \
    { \
        int32_t s = SUSEX16(SREG) + (imm); \
        GSU.vCarry = s >= 0x10000; \
        GSU.vOverflow = ~(SREG ^ (imm)) & ((imm) ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_add_i0 (void)  { FX_ADD_I(0); }
void fx_add_i1 (void)  { FX_ADD_I(1); }
void fx_add_i2 (void)  { FX_ADD_I(2); }
void fx_add_i3 (void)  { FX_ADD_I(3); }
void fx_add_i4 (void)  { FX_ADD_I(4); }
void fx_add_i5 (void)  { FX_ADD_I(5); }
void fx_add_i6 (void)  { FX_ADD_I(6); }
void fx_add_i7 (void)  { FX_ADD_I(7); }
void fx_add_i8 (void)  { FX_ADD_I(8); }
void fx_add_i9 (void)  { FX_ADD_I(9); }
void fx_add_i10 (void) { FX_ADD_I(10); }
void fx_add_i11 (void) { FX_ADD_I(11); }
void fx_add_i12 (void) { FX_ADD_I(12); }
void fx_add_i13 (void) { FX_ADD_I(13); }
void fx_add_i14 (void) { FX_ADD_I(14); }
void fx_add_i15 (void) { FX_ADD_I(15); }

#define FX_SUB(reg) \
    { \
        int32_t s = SUSEX16(SREG) - SUSEX16(GSU.avReg[reg]); \
        GSU.vCarry = s >= 0; \
        GSU.vOverflow = (SREG ^ GSU.avReg[reg]) & (SREG ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_sub_r0 (void)  { FX_SUB(0); }
void fx_sub_r1 (void)  { FX_SUB(1); }
void fx_sub_r2 (void)  { FX_SUB(2); }
void fx_sub_r3 (void)  { FX_SUB(3); }
void fx_sub_r4 (void)  { FX_SUB(4); }
void fx_sub_r5 (void)  { FX_SUB(5); }
void fx_sub_r6 (void)  { FX_SUB(6); }
void fx_sub_r7 (void)  { FX_SUB(7); }
void fx_sub_r8 (void)  { FX_SUB(8); }
void fx_sub_r9 (void)  { FX_SUB(9); }
void fx_sub_r10 (void) { FX_SUB(10); }
void fx_sub_r11 (void) { FX_SUB(11); }
void fx_sub_r12 (void) { FX_SUB(12); }
void fx_sub_r13 (void) { FX_SUB(13); }
void fx_sub_r14 (void) { FX_SUB(14); }
void fx_sub_r15 (void) { FX_SUB(15); }

#define FX_SUB_I(imm) \
    { \
        int32_t s = SUSEX16(SREG) - (imm); \
        GSU.vCarry = s >= 0; \
        GSU.vOverflow = (SREG ^ (imm)) & (SREG ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_sub_i0 (void)  { FX_SUB_I(0); }
void fx_sub_i1 (void)  { FX_SUB_I(1); }
void fx_sub_i2 (void)  { FX_SUB_I(2); }
void fx_sub_i3 (void)  { FX_SUB_I(3); }
void fx_sub_i4 (void)  { FX_SUB_I(4); }
void fx_sub_i5 (void)  { FX_SUB_I(5); }
void fx_sub_i6 (void)  { FX_SUB_I(6); }
void fx_sub_i7 (void)  { FX_SUB_I(7); }
void fx_sub_i8 (void)  { FX_SUB_I(8); }
void fx_sub_i9 (void)  { FX_SUB_I(9); }
void fx_sub_i10 (void) { FX_SUB_I(10); }
void fx_sub_i11 (void) { FX_SUB_I(11); }
void fx_sub_i12 (void) { FX_SUB_I(12); }
void fx_sub_i13 (void) { FX_SUB_I(13); }
void fx_sub_i14 (void) { FX_SUB_I(14); }
void fx_sub_i15 (void) { FX_SUB_I(15); }

#define FX_CMP(reg) \
    { \
        int32_t s = SUSEX16(SREG) - SUSEX16(GSU.avReg[reg]); \
        GSU.vCarry = s >= 0; \
        GSU.vOverflow = (SREG ^ GSU.avReg[reg]) & (SREG ^ s) & 0x8000; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        R15++; \
        CLRFLAGS; \
    }

void fx_cmp_r0 (void)  { FX_CMP(0); }
void fx_cmp_r1 (void)  { FX_CMP(1); }
void fx_cmp_r2 (void)  { FX_CMP(2); }
void fx_cmp_r3 (void)  { FX_CMP(3); }
void fx_cmp_r4 (void)  { FX_CMP(4); }
void fx_cmp_r5 (void)  { FX_CMP(5); }
void fx_cmp_r6 (void)  { FX_CMP(6); }
void fx_cmp_r7 (void)  { FX_CMP(7); }
void fx_cmp_r8 (void)  { FX_CMP(8); }
void fx_cmp_r9 (void)  { FX_CMP(9); }
void fx_cmp_r10 (void) { FX_CMP(10); }
void fx_cmp_r11 (void) { FX_CMP(11); }
void fx_cmp_r12 (void) { FX_CMP(12); }
void fx_cmp_r13 (void) { FX_CMP(13); }
void fx_cmp_r14 (void) { FX_CMP(14); }
void fx_cmp_r15 (void) { FX_CMP(15); }

#define FX_ADC(reg) \
    { \
        int32_t s = SUSEX16(SREG) + SUSEX16(GSU.avReg[reg]) + SUSEX16(GSU.vCarry); \
        GSU.vCarry = s >= 0x10000; \
        GSU.vOverflow = ~(SREG ^ GSU.avReg[reg]) & (GSU.avReg[reg] ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_adc_r0 (void)  { FX_ADC(0); }
void fx_adc_r1 (void)  { FX_ADC(1); }
void fx_adc_r2 (void)  { FX_ADC(2); }
void fx_adc_r3 (void)  { FX_ADC(3); }
void fx_adc_r4 (void)  { FX_ADC(4); }
void fx_adc_r5 (void)  { FX_ADC(5); }
void fx_adc_r6 (void)  { FX_ADC(6); }
void fx_adc_r7 (void)  { FX_ADC(7); }
void fx_adc_r8 (void)  { FX_ADC(8); }
void fx_adc_r9 (void)  { FX_ADC(9); }
void fx_adc_r10 (void) { FX_ADC(10); }
void fx_adc_r11 (void) { FX_ADC(11); }
void fx_adc_r12 (void) { FX_ADC(12); }
void fx_adc_r13 (void) { FX_ADC(13); }
void fx_adc_r14 (void) { FX_ADC(14); }
void fx_adc_r15 (void) { FX_ADC(15); }

#define FX_ADC_I(imm) \
    { \
        int32_t s = SUSEX16(SREG) + (imm) + SUSEX16(GSU.vCarry); \
        GSU.vCarry = s >= 0x10000; \
        GSU.vOverflow = ~(SREG ^ (imm)) & ((imm) ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_adc_i0 (void)  { FX_ADC_I(0); }
void fx_adc_i1 (void)  { FX_ADC_I(1); }
void fx_adc_i2 (void)  { FX_ADC_I(2); }
void fx_adc_i3 (void)  { FX_ADC_I(3); }
void fx_adc_i4 (void)  { FX_ADC_I(4); }
void fx_adc_i5 (void)  { FX_ADC_I(5); }
void fx_adc_i6 (void)  { FX_ADC_I(6); }
void fx_adc_i7 (void)  { FX_ADC_I(7); }
void fx_adc_i8 (void)  { FX_ADC_I(8); }
void fx_adc_i9 (void)  { FX_ADC_I(9); }
void fx_adc_i10 (void) { FX_ADC_I(10); }
void fx_adc_i11 (void) { FX_ADC_I(11); }
void fx_adc_i12 (void) { FX_ADC_I(12); }
void fx_adc_i13 (void) { FX_ADC_I(13); }
void fx_adc_i14 (void) { FX_ADC_I(14); }
void fx_adc_i15 (void) { FX_ADC_I(15); }

#define FX_SBC(reg) \
    { \
        int32_t s = SUSEX16(SREG) - SUSEX16(GSU.avReg[reg]) + GSU.vCarry - 1; \
        GSU.vCarry = s >= 0; \
        GSU.vOverflow = (SREG ^ GSU.avReg[reg]) & (SREG ^ s) & 0x8000; \
        R15++; \
        DREG = s; \
        GSU.vSign = s; \
        GSU.vZero = s; \
        TESTR14; \
        CLRFLAGS; \
    }

void fx_sbc_r0 (void)  { FX_SBC(0); }
void fx_sbc_r1 (void)  { FX_SBC(1); }
void fx_sbc_r2 (void)  { FX_SBC(2); }
void fx_sbc_r3 (void)  { FX_SBC(3); }
void fx_sbc_r4 (void)  { FX_SBC(4); }
void fx_sbc_r5 (void)  { FX_SBC(5); }
void fx_sbc_r6 (void)  { FX_SBC(6); }
void fx_sbc_r7 (void)  { FX_SBC(7); }
void fx_sbc_r8 (void)  { FX_SBC(8); }
void fx_sbc_r9 (void)  { FX_SBC(9); }
void fx_sbc_r10 (void) { FX_SBC(10); }
void fx_sbc_r11 (void) { FX_SBC(11); }
void fx_sbc_r12 (void) { FX_SBC(12); }
void fx_sbc_r13 (void) { FX_SBC(13); }
void fx_sbc_r14 (void) { FX_SBC(14); }
void fx_sbc_r15 (void) { FX_SBC(15); }

#define FX_MULT(reg) \
    uint32_t v = (uint32_t) (SEX8(SREG) * SEX8(GSU.avReg[reg])); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_mult_r0 (void)  { FX_MULT(0); }
void fx_mult_r1 (void)  { FX_MULT(1); }
void fx_mult_r2 (void)  { FX_MULT(2); }
void fx_mult_r3 (void)  { FX_MULT(3); }
void fx_mult_r4 (void)  { FX_MULT(4); }
void fx_mult_r5 (void)  { FX_MULT(5); }
void fx_mult_r6 (void)  { FX_MULT(6); }
void fx_mult_r7 (void)  { FX_MULT(7); }
void fx_mult_r8 (void)  { FX_MULT(8); }
void fx_mult_r9 (void)  { FX_MULT(9); }
void fx_mult_r10 (void) { FX_MULT(10); }
void fx_mult_r11 (void) { FX_MULT(11); }
void fx_mult_r12 (void) { FX_MULT(12); }
void fx_mult_r13 (void) { FX_MULT(13); }
void fx_mult_r14 (void) { FX_MULT(14); }
void fx_mult_r15 (void) { FX_MULT(15); }

#define FX_UMULT(reg) \
    uint32_t v = USEX8(SREG) * USEX8(GSU.avReg[reg]); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_umult_r0 (void)  { FX_UMULT(0); }
void fx_umult_r1 (void)  { FX_UMULT(1); }
void fx_umult_r2 (void)  { FX_UMULT(2); }
void fx_umult_r3 (void)  { FX_UMULT(3); }
void fx_umult_r4 (void)  { FX_UMULT(4); }
void fx_umult_r5 (void)  { FX_UMULT(5); }
void fx_umult_r6 (void)  { FX_UMULT(6); }
void fx_umult_r7 (void)  { FX_UMULT(7); }
void fx_umult_r8 (void)  { FX_UMULT(8); }
void fx_umult_r9 (void)  { FX_UMULT(9); }
void fx_umult_r10 (void) { FX_UMULT(10); }
void fx_umult_r11 (void) { FX_UMULT(11); }
void fx_umult_r12 (void) { FX_UMULT(12); }
void fx_umult_r13 (void) { FX_UMULT(13); }
void fx_umult_r14 (void) { FX_UMULT(14); }
void fx_umult_r15 (void) { FX_UMULT(15); }

#define FX_MULT_I(imm) \
    uint32_t v = (uint32_t) (SEX8(SREG) * ((int32_t) (imm))); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_mult_i0 (void)  { FX_MULT_I(0); }
void fx_mult_i1 (void)  { FX_MULT_I(1); }
void fx_mult_i2 (void)  { FX_MULT_I(2); }
void fx_mult_i3 (void)  { FX_MULT_I(3); }
void fx_mult_i4 (void)  { FX_MULT_I(4); }
void fx_mult_i5 (void)  { FX_MULT_I(5); }
void fx_mult_i6 (void)  { FX_MULT_I(6); }
void fx_mult_i7 (void)  { FX_MULT_I(7); }
void fx_mult_i8 (void)  { FX_MULT_I(8); }
void fx_mult_i9 (void)  { FX_MULT_I(9); }
void fx_mult_i10 (void) { FX_MULT_I(10); }
void fx_mult_i11 (void) { FX_MULT_I(11); }
void fx_mult_i12 (void) { FX_MULT_I(12); }
void fx_mult_i13 (void) { FX_MULT_I(13); }
void fx_mult_i14 (void) { FX_MULT_I(14); }
void fx_mult_i15 (void) { FX_MULT_I(15); }

#define FX_UMULT_I(imm) \
    uint32_t v = USEX8(SREG) * ((uint32_t) (imm)); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_umult_i0 (void)  { FX_UMULT_I(0); }
void fx_umult_i1 (void)  { FX_UMULT_I(1); }
void fx_umult_i2 (void)  { FX_UMULT_I(2); }
void fx_umult_i3 (void)  { FX_UMULT_I(3); }
void fx_umult_i4 (void)  { FX_UMULT_I(4); }
void fx_umult_i5 (void)  { FX_UMULT_I(5); }
void fx_umult_i6 (void)  { FX_UMULT_I(6); }
void fx_umult_i7 (void)  { FX_UMULT_I(7); }
void fx_umult_i8 (void)  { FX_UMULT_I(8); }
void fx_umult_i9 (void)  { FX_UMULT_I(9); }
void fx_umult_i10 (void) { FX_UMULT_I(10); }
void fx_umult_i11 (void) { FX_UMULT_I(11); }
void fx_umult_i12 (void) { FX_UMULT_I(12); }
void fx_umult_i13 (void) { FX_UMULT_I(13); }
void fx_umult_i14 (void) { FX_UMULT_I(14); }
void fx_umult_i15 (void) { FX_UMULT_I(15); }

void fx_fmult (void)
{
    uint32_t c = (uint32_t) (SEX16(SREG) * SEX16(R6));
    uint32_t v = c >> 16;
    R15++;
    R15++;
    DREG = v;
    GSU.vSign = v;
    GSU.vZero = v;
    GSU.vCarry = (c >> 15) & 1;
    TESTR14;
    CLRFLAGS;
}

void fx_lmult (void)
{
    uint32_t v = USEX16(SREG) * USEX16(R6);
    R4 = v & 0xffff;
    R15++;
    R15++;
    DREG = v >> 16;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
}
