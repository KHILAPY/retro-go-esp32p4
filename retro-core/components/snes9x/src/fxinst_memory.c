/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

#define FX_STW(reg) \
    GSU.vLastRamAdr = GSU.avReg[reg]; \
    RAM(GSU.avReg[reg]) = (uint8_t) SREG; \
    RAM(GSU.avReg[reg] ^ 1) = (uint8_t) (SREG >> 8); \
    CLRFLAGS; \
    R15++

#define FX_STB(reg) \
    GSU.vLastRamAdr = GSU.avReg[reg]; \
    RAM(GSU.avReg[reg]) = (uint8_t) SREG; \
    CLRFLAGS; \
    R15++

void fx_stw_r0 (void)  { FX_STW(0); }
void fx_stw_r1 (void)  { FX_STW(1); }
void fx_stw_r2 (void)  { FX_STW(2); }
void fx_stw_r3 (void)  { FX_STW(3); }
void fx_stw_r4 (void)  { FX_STW(4); }
void fx_stw_r5 (void)  { FX_STW(5); }
void fx_stw_r6 (void)  { FX_STW(6); }
void fx_stw_r7 (void)  { FX_STW(7); }
void fx_stw_r8 (void)  { FX_STW(8); }
void fx_stw_r9 (void)  { FX_STW(9); }
void fx_stw_r10 (void) { FX_STW(10); }
void fx_stw_r11 (void) { FX_STW(11); }

void fx_stb_r0 (void)  { FX_STB(0); }
void fx_stb_r1 (void)  { FX_STB(1); }
void fx_stb_r2 (void)  { FX_STB(2); }
void fx_stb_r3 (void)  { FX_STB(3); }
void fx_stb_r4 (void)  { FX_STB(4); }
void fx_stb_r5 (void)  { FX_STB(5); }
void fx_stb_r6 (void)  { FX_STB(6); }
void fx_stb_r7 (void)  { FX_STB(7); }
void fx_stb_r8 (void)  { FX_STB(8); }
void fx_stb_r9 (void)  { FX_STB(9); }
void fx_stb_r10 (void) { FX_STB(10); }
void fx_stb_r11 (void) { FX_STB(11); }

#define FX_LDW(reg) \
    GSU.vLastRamAdr = GSU.avReg[reg]; \
    DREG = (uint32_t) RAM(GSU.avReg[reg]) | ((uint32_t) RAM(GSU.avReg[reg] ^ 1) << 8); \
    GSU.vSign = DREG; \
    GSU.vZero = DREG; \
    CLRFLAGS; \
    R15++

#define FX_LDB(reg) \
    GSU.vLastRamAdr = GSU.avReg[reg]; \
    DREG = (uint32_t) RAM(GSU.avReg[reg]); \
    GSU.vSign = DREG; \
    GSU.vZero = DREG; \
    CLRFLAGS; \
    R15++

void fx_ldw_r0 (void)  { FX_LDW(0); }
void fx_ldw_r1 (void)  { FX_LDW(1); }
void fx_ldw_r2 (void)  { FX_LDW(2); }
void fx_ldw_r3 (void)  { FX_LDW(3); }
void fx_ldw_r4 (void)  { FX_LDW(4); }
void fx_ldw_r5 (void)  { FX_LDW(5); }
void fx_ldw_r6 (void)  { FX_LDW(6); }
void fx_ldw_r7 (void)  { FX_LDW(7); }
void fx_ldw_r8 (void)  { FX_LDW(8); }
void fx_ldw_r9 (void)  { FX_LDW(9); }
void fx_ldw_r10 (void) { FX_LDW(10); }
void fx_ldw_r11 (void) { FX_LDW(11); }

void fx_ldb_r0 (void)  { FX_LDB(0); }
void fx_ldb_r1 (void)  { FX_LDB(1); }
void fx_ldb_r2 (void)  { FX_LDB(2); }
void fx_ldb_r3 (void)  { FX_LDB(3); }
void fx_ldb_r4 (void)  { FX_LDB(4); }
void fx_ldb_r5 (void)  { FX_LDB(5); }
void fx_ldb_r6 (void)  { FX_LDB(6); }
void fx_ldb_r7 (void)  { FX_LDB(7); }
void fx_ldb_r8 (void)  { FX_LDB(8); }
void fx_ldb_r9 (void)  { FX_LDB(9); }
void fx_ldb_r10 (void) { FX_LDB(10); }
void fx_ldb_r11 (void) { FX_LDB(11); }

#define FX_LMS(reg) \
    GSU.vLastRamAdr = ((uint32_t) PIPE) << 1; \
    R15++; \
    FETCHPIPE; \
    R15++; \
    GSU.avReg[reg] = (uint32_t) RAM(GSU.vLastRamAdr); \
    GSU.avReg[reg] |= ((uint32_t) RAM(GSU.vLastRamAdr + 1)) << 8; \
    CLRFLAGS

void fx_lms_r0 (void)  { FX_LMS(0); }
void fx_lms_r1 (void)  { FX_LMS(1); }
void fx_lms_r2 (void)  { FX_LMS(2); }
void fx_lms_r3 (void)  { FX_LMS(3); }
void fx_lms_r4 (void)  { FX_LMS(4); }
void fx_lms_r5 (void)  { FX_LMS(5); }
void fx_lms_r6 (void)  { FX_LMS(6); }
void fx_lms_r7 (void)  { FX_LMS(7); }
void fx_lms_r8 (void)  { FX_LMS(8); }
void fx_lms_r9 (void)  { FX_LMS(9); }
void fx_lms_r10 (void) { FX_LMS(10); }
void fx_lms_r11 (void) { FX_LMS(11); }
void fx_lms_r12 (void) { FX_LMS(12); }
void fx_lms_r13 (void) { FX_LMS(13); }
void fx_lms_r14 (void) { FX_LMS(14); READR14; }
void fx_lms_r15 (void) { FX_LMS(15); }

#define FX_SMS(reg) \
    uint32_t v = GSU.avReg[reg]; \
    GSU.vLastRamAdr = ((uint32_t) PIPE) << 1; \
    R15++; \
    FETCHPIPE; \
    RAM(GSU.vLastRamAdr) = (uint8_t) v; \
    RAM(GSU.vLastRamAdr + 1) = (uint8_t) (v >> 8); \
    R15++; \
    CLRFLAGS

void fx_sms_r0 (void)  { FX_SMS(0); }
void fx_sms_r1 (void)  { FX_SMS(1); }
void fx_sms_r2 (void)  { FX_SMS(2); }
void fx_sms_r3 (void)  { FX_SMS(3); }
void fx_sms_r4 (void)  { FX_SMS(4); }
void fx_sms_r5 (void)  { FX_SMS(5); }
void fx_sms_r6 (void)  { FX_SMS(6); }
void fx_sms_r7 (void)  { FX_SMS(7); }
void fx_sms_r8 (void)  { FX_SMS(8); }
void fx_sms_r9 (void)  { FX_SMS(9); }
void fx_sms_r10 (void) { FX_SMS(10); }
void fx_sms_r11 (void) { FX_SMS(11); }
void fx_sms_r12 (void) { FX_SMS(12); }
void fx_sms_r13 (void) { FX_SMS(13); }
void fx_sms_r14 (void) { FX_SMS(14); }
void fx_sms_r15 (void) { FX_SMS(15); }

#define FX_LM(reg) \
    GSU.vLastRamAdr = PIPE; \
    R15++; \
    FETCHPIPE; \
    R15++; \
    GSU.vLastRamAdr |= USEX8(PIPE) << 8; \
    FETCHPIPE; \
    R15++; \
    GSU.avReg[reg] = RAM(GSU.vLastRamAdr); \
    GSU.avReg[reg] |= USEX8(RAM(GSU.vLastRamAdr ^ 1)) << 8; \
    CLRFLAGS

void fx_lm_r0 (void)  { FX_LM(0); }
void fx_lm_r1 (void)  { FX_LM(1); }
void fx_lm_r2 (void)  { FX_LM(2); }
void fx_lm_r3 (void)  { FX_LM(3); }
void fx_lm_r4 (void)  { FX_LM(4); }
void fx_lm_r5 (void)  { FX_LM(5); }
void fx_lm_r6 (void)  { FX_LM(6); }
void fx_lm_r7 (void)  { FX_LM(7); }
void fx_lm_r8 (void)  { FX_LM(8); }
void fx_lm_r9 (void)  { FX_LM(9); }
void fx_lm_r10 (void) { FX_LM(10); }
void fx_lm_r11 (void) { FX_LM(11); }
void fx_lm_r12 (void) { FX_LM(12); }
void fx_lm_r13 (void) { FX_LM(13); }
void fx_lm_r14 (void) { FX_LM(14); READR14; }
void fx_lm_r15 (void) { FX_LM(15); }

#define FX_SM(reg) \
    uint32_t v = GSU.avReg[reg]; \
    GSU.vLastRamAdr = PIPE; \
    R15++; \
    FETCHPIPE; \
    R15++; \
    GSU.vLastRamAdr |= USEX8(PIPE) << 8; \
    FETCHPIPE; \
    RAM(GSU.vLastRamAdr) = (uint8_t) v; \
    RAM(GSU.vLastRamAdr ^ 1) = (uint8_t) (v >> 8); \
    CLRFLAGS; \
    R15++

void fx_sm_r0 (void)  { FX_SM(0); }
void fx_sm_r1 (void)  { FX_SM(1); }
void fx_sm_r2 (void)  { FX_SM(2); }
void fx_sm_r3 (void)  { FX_SM(3); }
void fx_sm_r4 (void)  { FX_SM(4); }
void fx_sm_r5 (void)  { FX_SM(5); }
void fx_sm_r6 (void)  { FX_SM(6); }
void fx_sm_r7 (void)  { FX_SM(7); }
void fx_sm_r8 (void)  { FX_SM(8); }
void fx_sm_r9 (void)  { FX_SM(9); }
void fx_sm_r10 (void) { FX_SM(10); }
void fx_sm_r11 (void) { FX_SM(11); }
void fx_sm_r12 (void) { FX_SM(12); }
void fx_sm_r13 (void) { FX_SM(13); }
void fx_sm_r14 (void) { FX_SM(14); }
void fx_sm_r15 (void) { FX_SM(15); }

void fx_ramb (void)
{
    RAMBR = SREG & 3;
    GSU.pvRamBank = GSU.apvRamBank[RAMBR];
    CLRFLAGS;
    R15++;
}

void fx_romb (void)
{
    ROMBR = SREG & 0x7f;
    GSU.pvRomBank = GSU.apvRomBank[ROMBR];
    CLRFLAGS;
    R15++;
}
