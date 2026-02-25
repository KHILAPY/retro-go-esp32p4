/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

void fx_lsr (void)
{
    uint32_t v;
    GSU.vCarry = SREG & 1;
    v = USEX16(SREG) >> 1;
    R15++;
    DREG = v;
    GSU.vSign = v;
    GSU.vZero = v;
    TESTR14;
    CLRFLAGS;
}

void fx_rol (void)
{
    uint32_t v = USEX16((SREG << 1) + GSU.vCarry);
    GSU.vCarry = (SREG >> 15) & 1;
    R15++;
    DREG = v;
    GSU.vSign = v;
    GSU.vZero = v;
    TESTR14;
    CLRFLAGS;
}

void fx_ror (void)
{
    uint32_t v = (USEX16(SREG) >> 1) + (GSU.vCarry << 15);
    GSU.vCarry = SREG & 1;
    R15++;
    DREG = v;
    GSU.vSign = v;
    GSU.vZero = v;
    TESTR14;
    CLRFLAGS;
}

void fx_asr (void)
{
    GSU.vCarry = SREG & 1;
    DREG = SEX16(SREG) >> 1;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

void fx_div2 (void)
{
    int32_t v = SUSEX16(SREG);
    GSU.vCarry = v & 1;
    v >>= 1;
    if ((SREG & 0x8000) && !(SREG & 0x7fff))
        v++;
    R15++;
    DREG = v;
    GSU.vSign = v;
    GSU.vZero = v;
    TESTR14;
    CLRFLAGS;
}

void fx_sex (void)
{
    DREG = SEX16(SREG);
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

void fx_swap (void)
{
    DREG = ((SREG & 0xff) << 8) | ((SREG >> 8) & 0xff);
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

void fx_lob (void)
{
    DREG = SREG & 0xff;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

void fx_hib (void)
{
    DREG = (SREG >> 8) & 0xff;
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    TESTR14;
    CLRFLAGS;
    R15++;
}

void fx_merge (void)
{
    DREG = (R7 & 0xff00) | ((R8 & 0xff00) >> 8);
    GSU.vSign = DREG;
    GSU.vZero = DREG;
    GSU.vOverflow = (DREG & 0xc0c0) << 16;
    GSU.vCarry = (DREG & 0xe0e0) != 0;
    TESTR14;
    CLRFLAGS;
    R15++;
}
