/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

#define FX_TO(reg) \
    if (TF(B)) \
    { \
        GSU.avReg[(reg)] = SREG; \
        CLRFLAGS; \
    } \
    else \
        GSU.pvDreg = &GSU.avReg[reg]; \
    R15++

#define FX_TO_R14(reg) \
    if (TF(B)) \
    { \
        GSU.avReg[(reg)] = SREG; \
        CLRFLAGS; \
        READR14; \
    } \
    else \
        GSU.pvDreg = &GSU.avReg[reg]; \
    R15++

#define FX_TO_R15(reg) \
    if (TF(B)) \
    { \
        GSU.avReg[(reg)] = SREG; \
        CLRFLAGS; \
    } \
    else \
    { \
        GSU.pvDreg = &GSU.avReg[reg]; \
        R15++; \
    }

void fx_to_r0 (void)  { FX_TO(0); }
void fx_to_r1 (void)  { FX_TO(1); }
void fx_to_r2 (void)  { FX_TO(2); }
void fx_to_r3 (void)  { FX_TO(3); }
void fx_to_r4 (void)  { FX_TO(4); }
void fx_to_r5 (void)  { FX_TO(5); }
void fx_to_r6 (void)  { FX_TO(6); }
void fx_to_r7 (void)  { FX_TO(7); }
void fx_to_r8 (void)  { FX_TO(8); }
void fx_to_r9 (void)  { FX_TO(9); }
void fx_to_r10 (void) { FX_TO(10); }
void fx_to_r11 (void) { FX_TO(11); }
void fx_to_r12 (void) { FX_TO(12); }
void fx_to_r13 (void) { FX_TO(13); }
void fx_to_r14 (void) { FX_TO_R14(14); }
void fx_to_r15 (void) { FX_TO_R15(15); }

#define FX_WITH(reg) \
    SF(B); \
    GSU.pvSreg = GSU.pvDreg = &GSU.avReg[reg]; \
    R15++

void fx_with_r0 (void)  { FX_WITH(0); }
void fx_with_r1 (void)  { FX_WITH(1); }
void fx_with_r2 (void)  { FX_WITH(2); }
void fx_with_r3 (void)  { FX_WITH(3); }
void fx_with_r4 (void)  { FX_WITH(4); }
void fx_with_r5 (void)  { FX_WITH(5); }
void fx_with_r6 (void)  { FX_WITH(6); }
void fx_with_r7 (void)  { FX_WITH(7); }
void fx_with_r8 (void)  { FX_WITH(8); }
void fx_with_r9 (void)  { FX_WITH(9); }
void fx_with_r10 (void) { FX_WITH(10); }
void fx_with_r11 (void) { FX_WITH(11); }
void fx_with_r12 (void) { FX_WITH(12); }
void fx_with_r13 (void) { FX_WITH(13); }
void fx_with_r14 (void) { FX_WITH(14); }
void fx_with_r15 (void) { FX_WITH(15); }

#define FX_FROM(reg) \
    if (TF(B)) \
    { \
        uint32_t v = GSU.avReg[reg]; \
        R15++; \
        DREG = v; \
        GSU.vOverflow = (v & 0x80) << 16; \
        GSU.vSign = v; \
        GSU.vZero = v; \
        TESTR14; \
        CLRFLAGS; \
    } \
    else \
    { \
        GSU.pvSreg = &GSU.avReg[reg]; \
        R15++; \
    }

void fx_from_r0 (void)  { FX_FROM(0); }
void fx_from_r1 (void)  { FX_FROM(1); }
void fx_from_r2 (void)  { FX_FROM(2); }
void fx_from_r3 (void)  { FX_FROM(3); }
void fx_from_r4 (void)  { FX_FROM(4); }
void fx_from_r5 (void)  { FX_FROM(5); }
void fx_from_r6 (void)  { FX_FROM(6); }
void fx_from_r7 (void)  { FX_FROM(7); }
void fx_from_r8 (void)  { FX_FROM(8); }
void fx_from_r9 (void)  { FX_FROM(9); }
void fx_from_r10 (void) { FX_FROM(10); }
void fx_from_r11 (void) { FX_FROM(11); }
void fx_from_r12 (void) { FX_FROM(12); }
void fx_from_r13 (void) { FX_FROM(13); }
void fx_from_r14 (void) { FX_FROM(14); READR14; }
void fx_from_r15 (void) { FX_FROM(15); }

#define FX_INC(reg) \
    GSU.avReg[reg] += 1; \
    GSU.vSign = GSU.avReg[reg]; \
    GSU.vZero = GSU.avReg[reg]; \
    CLRFLAGS; \
    R15++

void fx_inc_r0 (void)  { FX_INC(0); }
void fx_inc_r1 (void)  { FX_INC(1); }
void fx_inc_r2 (void)  { FX_INC(2); }
void fx_inc_r3 (void)  { FX_INC(3); }
void fx_inc_r4 (void)  { FX_INC(4); }
void fx_inc_r5 (void)  { FX_INC(5); }
void fx_inc_r6 (void)  { FX_INC(6); }
void fx_inc_r7 (void)  { FX_INC(7); }
void fx_inc_r8 (void)  { FX_INC(8); }
void fx_inc_r9 (void)  { FX_INC(9); }
void fx_inc_r10 (void) { FX_INC(10); }
void fx_inc_r11 (void) { FX_INC(11); }
void fx_inc_r12 (void) { FX_INC(12); }
void fx_inc_r13 (void) { FX_INC(13); }
void fx_inc_r14 (void) { FX_INC(14); READR14; }

#define FX_DEC(reg) \
    GSU.avReg[reg] -= 1; \
    GSU.vSign = GSU.avReg[reg]; \
    GSU.vZero = GSU.avReg[reg]; \
    CLRFLAGS; \
    R15++

void fx_dec_r0 (void)  { FX_DEC(0); }
void fx_dec_r1 (void)  { FX_DEC(1); }
void fx_dec_r2 (void)  { FX_DEC(2); }
void fx_dec_r3 (void)  { FX_DEC(3); }
void fx_dec_r4 (void)  { FX_DEC(4); }
void fx_dec_r5 (void)  { FX_DEC(5); }
void fx_dec_r6 (void)  { FX_DEC(6); }
void fx_dec_r7 (void)  { FX_DEC(7); }
void fx_dec_r8 (void)  { FX_DEC(8); }
void fx_dec_r9 (void)  { FX_DEC(9); }
void fx_dec_r10 (void) { FX_DEC(10); }
void fx_dec_r11 (void) { FX_DEC(11); }
void fx_dec_r12 (void) { FX_DEC(12); }
void fx_dec_r13 (void) { FX_DEC(13); }
void fx_dec_r14 (void) { FX_DEC(14); READR14; }

#define FX_IBT(reg) \
    GSU.avReg[reg] = SEX8(PIPE); \
    R15++; \
    FETCHPIPE

void fx_ibt_r0 (void)  { FX_IBT(0); }
void fx_ibt_r1 (void)  { FX_IBT(1); }
void fx_ibt_r2 (void)  { FX_IBT(2); }
void fx_ibt_r3 (void)  { FX_IBT(3); }
void fx_ibt_r4 (void)  { FX_IBT(4); }
void fx_ibt_r5 (void)  { FX_IBT(5); }
void fx_ibt_r6 (void)  { FX_IBT(6); }
void fx_ibt_r7 (void)  { FX_IBT(7); }
void fx_ibt_r8 (void)  { FX_IBT(8); }
void fx_ibt_r9 (void)  { FX_IBT(9); }
void fx_ibt_r10 (void) { FX_IBT(10); }
void fx_ibt_r11 (void) { FX_IBT(11); }
void fx_ibt_r12 (void) { FX_IBT(12); }
void fx_ibt_r13 (void) { FX_IBT(13); }
void fx_ibt_r14 (void) { FX_IBT(14); READR14; }
void fx_ibt_r15 (void) { FX_IBT(15); }

#define FX_IWT(reg) \
    { \
        uint8_t lo = PIPE; \
        R15++; \
        FETCHPIPE; \
        uint8_t hi = PIPE; \
        R15++; \
        FETCHPIPE; \
        GSU.avReg[reg] = lo | (hi << 8); \
    }

void fx_iwt_r0 (void)  { FX_IWT(0); }
void fx_iwt_r1 (void)  { FX_IWT(1); }
void fx_iwt_r2 (void)  { FX_IWT(2); }
void fx_iwt_r3 (void)  { FX_IWT(3); }
void fx_iwt_r4 (void)  { FX_IWT(4); }
void fx_iwt_r5 (void)  { FX_IWT(5); }
void fx_iwt_r6 (void)  { FX_IWT(6); }
void fx_iwt_r7 (void)  { FX_IWT(7); }
void fx_iwt_r8 (void)  { FX_IWT(8); }
void fx_iwt_r9 (void)  { FX_IWT(9); }
void fx_iwt_r10 (void) { FX_IWT(10); }
void fx_iwt_r11 (void) { FX_IWT(11); }
void fx_iwt_r12 (void) { FX_IWT(12); }
void fx_iwt_r13 (void) { FX_IWT(13); }
void fx_iwt_r14 (void) { FX_IWT(14); READR14; }
void fx_iwt_r15 (void) { FX_IWT(15); }

#define FX_LINK(n) \
    R11 = R15 + n; \
    CLRFLAGS; \
    R15++

void fx_link_i1 (void) { FX_LINK(1); }
void fx_link_i2 (void) { FX_LINK(2); }
void fx_link_i3 (void) { FX_LINK(3); }
void fx_link_i4 (void) { FX_LINK(4); }
