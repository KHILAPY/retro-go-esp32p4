/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

void fx_bra (void)
{
    uint8_t v = PIPE;
    R15++;
    FETCHPIPE;
    CLRFLAGS;
    R15 += SEX8(v);
}

void fx_blt (void) { BRA_COND((TEST_S != 0) != (TEST_OV != 0)); }
void fx_bge (void) { BRA_COND((TEST_S != 0) == (TEST_OV != 0)); }
void fx_bne (void) { BRA_COND(!TEST_Z); }
void fx_beq (void) { BRA_COND(TEST_Z); }
void fx_bpl (void) { BRA_COND(!TEST_S); }
void fx_bmi (void) { BRA_COND(TEST_S); }
void fx_bcc (void) { BRA_COND(!TEST_CY); }
void fx_bcs (void) { BRA_COND(TEST_CY); }
void fx_bvc (void) { BRA_COND(!TEST_OV); }
void fx_bvs (void) { BRA_COND(TEST_OV); }

void fx_jmp_r8 (void)  { CLRFLAGS; R15 = R8; FETCHPIPE }
void fx_jmp_r9 (void)  { CLRFLAGS; R15 = R9; FETCHPIPE }
void fx_jmp_r10 (void) { CLRFLAGS; R15 = R10; FETCHPIPE }
void fx_jmp_r11 (void) { CLRFLAGS; R15 = R11; FETCHPIPE }
void fx_jmp_r12 (void) { CLRFLAGS; R15 = R12; FETCHPIPE }
void fx_jmp_r13 (void) { CLRFLAGS; R15 = R13; FETCHPIPE }

void fx_ljmp_r8 (void)  { CLRFLAGS; PBR = R8 >> 16; R15 = R8; FETCHPIPE }
void fx_ljmp_r9 (void)  { CLRFLAGS; PBR = R9 >> 16; R15 = R9; FETCHPIPE }
void fx_ljmp_r10 (void) { CLRFLAGS; PBR = R10 >> 16; R15 = R10; FETCHPIPE }
void fx_ljmp_r11 (void) { CLRFLAGS; PBR = R11 >> 16; R15 = R11; FETCHPIPE }
void fx_ljmp_r12 (void) { CLRFLAGS; PBR = R12 >> 16; R15 = R12; FETCHPIPE }
void fx_ljmp_r13 (void) { CLRFLAGS; PBR = R13 >> 16; R15 = R13; FETCHPIPE }
