/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

void fx_not (void)
{
    DREG = ~SREG;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

#define FX_AND(reg) \
    DREG = SREG & GSU.avReg[reg]; \
    GSU.vSign = DREG; \
    GSU.vZero = DREG; \
    TESTR14; \
    CLRFLAGS; \
    R15++

void fx_and_r1 (void)  { FX_AND(1); }
void fx_and_r2 (void)  { FX_AND(2); }
void fx_and_r3 (void)  { FX_AND(3); }
void fx_and_r4 (void)  { FX_AND(4); }
void fx_and_r5 (void)  { FX_AND(5); }
void fx_and_r6 (void)  { FX_AND(6); }
void fx_and_r7 (void)  { FX_AND(7); }
void fx_and_r8 (void)  { FX_AND(8); }
void fx_and_r9 (void)  { FX_AND(9); }
void fx_and_r10 (void) { FX_AND(10); }
void fx_and_r11 (void) { FX_AND(11); }
void fx_and_r12 (void) { FX_AND(12); }
void fx_and_r13 (void) { FX_AND(13); }
void fx_and_r14 (void) { FX_AND(14); }
void fx_and_r15 (void) { FX_AND(15); }

#define FX_AND_I(imm) \
    uint32_t v = SREG & (imm); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_and_i1 (void)  { FX_AND_I(1); }
void fx_and_i2 (void)  { FX_AND_I(2); }
void fx_and_i3 (void)  { FX_AND_I(3); }
void fx_and_i4 (void)  { FX_AND_I(4); }
void fx_and_i5 (void)  { FX_AND_I(5); }
void fx_and_i6 (void)  { FX_AND_I(6); }
void fx_and_i7 (void)  { FX_AND_I(7); }
void fx_and_i8 (void)  { FX_AND_I(8); }
void fx_and_i9 (void)  { FX_AND_I(9); }
void fx_and_i10 (void) { FX_AND_I(10); }
void fx_and_i11 (void) { FX_AND_I(11); }
void fx_and_i12 (void) { FX_AND_I(12); }
void fx_and_i13 (void) { FX_AND_I(13); }
void fx_and_i14 (void) { FX_AND_I(14); }
void fx_and_i15 (void) { FX_AND_I(15); }

#define FX_BIC(reg) \
    DREG = SREG & ~GSU.avReg[reg]; \
    GSU.vSign = DREG; \
    GSU.vZero = DREG; \
    TESTR14; \
    CLRFLAGS; \
    R15++

void fx_bic_r1 (void)  { FX_BIC(1); }
void fx_bic_r2 (void)  { FX_BIC(2); }
void fx_bic_r3 (void)  { FX_BIC(3); }
void fx_bic_r4 (void)  { FX_BIC(4); }
void fx_bic_r5 (void)  { FX_BIC(5); }
void fx_bic_r6 (void)  { FX_BIC(6); }
void fx_bic_r7 (void)  { FX_BIC(7); }
void fx_bic_r8 (void)  { FX_BIC(8); }
void fx_bic_r9 (void)  { FX_BIC(9); }
void fx_bic_r10 (void) { FX_BIC(10); }
void fx_bic_r11 (void) { FX_BIC(11); }
void fx_bic_r12 (void) { FX_BIC(12); }
void fx_bic_r13 (void) { FX_BIC(13); }
void fx_bic_r14 (void) { FX_BIC(14); }
void fx_bic_r15 (void) { FX_BIC(15); }

#define FX_BIC_I(imm) \
    uint32_t v = SREG & ~(imm); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_bic_i1 (void)  { FX_BIC_I(1); }
void fx_bic_i2 (void)  { FX_BIC_I(2); }
void fx_bic_i3 (void)  { FX_BIC_I(3); }
void fx_bic_i4 (void)  { FX_BIC_I(4); }
void fx_bic_i5 (void)  { FX_BIC_I(5); }
void fx_bic_i6 (void)  { FX_BIC_I(6); }
void fx_bic_i7 (void)  { FX_BIC_I(7); }
void fx_bic_i8 (void)  { FX_BIC_I(8); }
void fx_bic_i9 (void)  { FX_BIC_I(9); }
void fx_bic_i10 (void) { FX_BIC_I(10); }
void fx_bic_i11 (void) { FX_BIC_I(11); }
void fx_bic_i12 (void) { FX_BIC_I(12); }
void fx_bic_i13 (void) { FX_BIC_I(13); }
void fx_bic_i14 (void) { FX_BIC_I(14); }
void fx_bic_i15 (void) { FX_BIC_I(15); }

#define FX_OR(reg) \
    DREG = SREG | GSU.avReg[reg]; \
    GSU.vSign = DREG; \
    GSU.vZero = DREG; \
    TESTR14; \
    CLRFLAGS; \
    R15++

void fx_or_r1 (void)  { FX_OR(1); }
void fx_or_r2 (void)  { FX_OR(2); }
void fx_or_r3 (void)  { FX_OR(3); }
void fx_or_r4 (void)  { FX_OR(4); }
void fx_or_r5 (void)  { FX_OR(5); }
void fx_or_r6 (void)  { FX_OR(6); }
void fx_or_r7 (void)  { FX_OR(7); }
void fx_or_r8 (void)  { FX_OR(8); }
void fx_or_r9 (void)  { FX_OR(9); }
void fx_or_r10 (void) { FX_OR(10); }
void fx_or_r11 (void) { FX_OR(11); }
void fx_or_r12 (void) { FX_OR(12); }
void fx_or_r13 (void) { FX_OR(13); }
void fx_or_r14 (void) { FX_OR(14); }
void fx_or_r15 (void) { FX_OR(15); }

#define FX_OR_I(imm) \
    uint32_t v = SREG | (imm); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_or_i1 (void)  { FX_OR_I(1); }
void fx_or_i2 (void)  { FX_OR_I(2); }
void fx_or_i3 (void)  { FX_OR_I(3); }
void fx_or_i4 (void)  { FX_OR_I(4); }
void fx_or_i5 (void)  { FX_OR_I(5); }
void fx_or_i6 (void)  { FX_OR_I(6); }
void fx_or_i7 (void)  { FX_OR_I(7); }
void fx_or_i8 (void)  { FX_OR_I(8); }
void fx_or_i9 (void)  { FX_OR_I(9); }
void fx_or_i10 (void) { FX_OR_I(10); }
void fx_or_i11 (void) { FX_OR_I(11); }
void fx_or_i12 (void) { FX_OR_I(12); }
void fx_or_i13 (void) { FX_OR_I(13); }
void fx_or_i14 (void) { FX_OR_I(14); }
void fx_or_i15 (void) { FX_OR_I(15); }

#define FX_XOR(reg) \
    DREG = SREG ^ GSU.avReg[reg]; \
    GSU.vSign = DREG; \
    GSU.vZero = DREG; \
    TESTR14; \
    CLRFLAGS; \
    R15++

void fx_xor_r1 (void)  { FX_XOR(1); }
void fx_xor_r2 (void)  { FX_XOR(2); }
void fx_xor_r3 (void)  { FX_XOR(3); }
void fx_xor_r4 (void)  { FX_XOR(4); }
void fx_xor_r5 (void)  { FX_XOR(5); }
void fx_xor_r6 (void)  { FX_XOR(6); }
void fx_xor_r7 (void)  { FX_XOR(7); }
void fx_xor_r8 (void)  { FX_XOR(8); }
void fx_xor_r9 (void)  { FX_XOR(9); }
void fx_xor_r10 (void) { FX_XOR(10); }
void fx_xor_r11 (void) { FX_XOR(11); }
void fx_xor_r12 (void) { FX_XOR(12); }
void fx_xor_r13 (void) { FX_XOR(13); }
void fx_xor_r14 (void) { FX_XOR(14); }
void fx_xor_r15 (void) { FX_XOR(15); }

#define FX_XOR_I(imm) \
    uint32_t v = SREG ^ (imm); \
    R15++; \
    DREG = v; \
    GSU.vSign = v; \
    GSU.vZero = v; \
    TESTR14; \
    CLRFLAGS

void fx_xor_i1 (void)  { FX_XOR_I(1); }
void fx_xor_i2 (void)  { FX_XOR_I(2); }
void fx_xor_i3 (void)  { FX_XOR_I(3); }
void fx_xor_i4 (void)  { FX_XOR_I(4); }
void fx_xor_i5 (void)  { FX_XOR_I(5); }
void fx_xor_i6 (void)  { FX_XOR_I(6); }
void fx_xor_i7 (void)  { FX_XOR_I(7); }
void fx_xor_i8 (void)  { FX_XOR_I(8); }
void fx_xor_i9 (void)  { FX_XOR_I(9); }
void fx_xor_i10 (void) { FX_XOR_I(10); }
void fx_xor_i11 (void) { FX_XOR_I(11); }
void fx_xor_i12 (void) { FX_XOR_I(12); }
void fx_xor_i13 (void) { FX_XOR_I(13); }
void fx_xor_i14 (void) { FX_XOR_I(14); }
void fx_xor_i15 (void) { FX_XOR_I(15); }
