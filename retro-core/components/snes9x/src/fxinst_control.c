/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "fxinst_internal.h"

void fx_stop (void)
{
    CF(G);
    GSU.vCounter = 0;
    GSU.vInstCount = GSU.vCounter;
    if (!(GSU.pvRegisters[GSU_CFGR] & 0x80))
        SF(IRQ);
    GSU.vPlotOptionReg = 0;
    GSU.vPipe = 1;
    CLRFLAGS;
    R15++;
}

void fx_nop (void)
{
    CLRFLAGS;
    R15++;
}

void fx_cache (void)
{
    uint32_t c = R15 & 0xfff0;
    if (GSU.vCacheBaseReg != c || !GSU.bCacheActive)
    {
        fx_flushCache();
        GSU.vCacheBaseReg = c;
        GSU.bCacheActive = true;
    }
    CLRFLAGS;
    R15++;
}

void fx_loop (void)
{
    GSU.vSign = --R12;
    GSU.vZero = R12;
    if ((uint16_t) R12 != 0)
    {
        R15 = R13;
        FETCHPIPE;
    }
    else
    {
        R15++;
        FETCHPIPE;
    }
    CLRFLAGS;
}

void fx_alt1 (void)
{
    CLRFLAGS;
    SF(ALT1);
    R15++;
}

void fx_alt2 (void)
{
    CLRFLAGS;
    SF(ALT2);
    R15++;
}

void fx_alt3 (void)
{
    CLRFLAGS;
    SF(ALT1);
    SF(ALT2);
    R15++;
}

void fx_sbk (void)
{
    RAM(GSU.vLastRamAdr) = (uint8_t) SREG;
    RAM(GSU.vLastRamAdr ^ 1) = (uint8_t) (SREG >> 8);
    CLRFLAGS;
    R15++;
}
