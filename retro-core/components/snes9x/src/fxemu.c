/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include "snes9x.h"
#include "memmap.h"
#include "cpuexec.h"
#include "fxinst.h"
#include "fxemu.h"

#include <stdio.h>

#ifdef RETRO_GO
#include <rg_system.h>
#endif

struct FxRegs_s  GSU;
struct FxInfo_s  SuperFX;

static void FxReset (struct FxInfo_s *);
static void fx_readRegisterSpace (void);
static void fx_writeRegisterSpace (void);
static void fx_updateRamBank (uint8_t);
static void fx_dirtySCBR (void);
static bool fx_checkStartAddress (void);
static uint32_t FxEmulate (uint32_t);
static void FxCacheWriteAccess (uint16_t);
static void FxFlushCache (void);

void S9xInitSuperFX (void)
{
    memset((uint8_t *) &GSU, 0, sizeof(struct FxRegs_s));
}

void S9xResetSuperFX (void)
{
    SuperFX.speedPerLine = (uint32_t) (5823405 * ((1.0 / (float) Memory.ROMFramesPerSecond) / 262.0));
    SuperFX.oneLineDone = false;
    SuperFX.vFlags = 0;
    FxReset(&SuperFX);
}

void S9xSetSuperFX (uint8_t byte, uint16_t address)
{
    switch (address)
    {
        case 0x3030:
            if ((Memory.FillRAM[0x3030] ^ byte) & FLG_G)
            {
                Memory.FillRAM[0x3030] = byte;
                if (byte & FLG_G)
                {
                    if (!SuperFX.oneLineDone)
                    {
                        S9xSuperFXExec();
                        SuperFX.oneLineDone = true;
                    }
                }
                else
                    FxFlushCache();
            }
            else
                Memory.FillRAM[0x3030] = byte;

            break;

        case 0x3031:
            Memory.FillRAM[0x3031] = byte;
            break;

        case 0x3033:
            Memory.FillRAM[0x3033] = byte;
            break;

        case 0x3034:
            Memory.FillRAM[0x3034] = byte & 0x7f;
            break;

        case 0x3036:
            Memory.FillRAM[0x3036] = byte & 0x7f;
            break;

        case 0x3037:
            Memory.FillRAM[0x3037] = byte;
            break;

        case 0x3038:
            Memory.FillRAM[0x3038] = byte;
            fx_dirtySCBR();
            break;

        case 0x3039:
            Memory.FillRAM[0x3039] = byte;
            break;

        case 0x303a:
            Memory.FillRAM[0x303a] = byte;
            break;

        case 0x303b:
            break;

        case 0x303c:
            Memory.FillRAM[0x303c] = byte;
            fx_updateRamBank(byte);
            break;

        case 0x303f:
            Memory.FillRAM[0x303f] = byte;
            break;

        case 0x301f:
            Memory.FillRAM[0x301f] = byte;
            Memory.FillRAM[0x3000 + GSU_SFR] |= FLG_G;
            if (!SuperFX.oneLineDone)
            {
                S9xSuperFXExec();
                SuperFX.oneLineDone = true;
            }

            break;

        default:
            Memory.FillRAM[address] = byte;
            if (address >= 0x3100)
                FxCacheWriteAccess(address);

            break;
    }
}

uint8_t S9xGetSuperFX (uint16_t address)
{
    uint8_t    byte;

    byte = Memory.FillRAM[address];

    if (address == 0x3031)
    {
        CLEAR_IRQ_SOURCE(GSU_IRQ_SOURCE);
        Memory.FillRAM[0x3031] = byte & 0x7f;
    }

    return (byte);
}

void S9xSuperFXExec (void)
{
    uint8_t sfr_val = Memory.FillRAM[0x3000 + GSU_SFR];
    uint8_t scmr_val = Memory.FillRAM[0x3000 + GSU_SCMR];
    static uint32_t exec_count = 0;

    if ((sfr_val & FLG_G) && (scmr_val & 0x18) != 0)
    {
        uint32_t nInst = ((Memory.FillRAM[0x3000 + GSU_CLSR] & 1) ? (SuperFX.speedPerLine * 5 / 2) : SuperFX.speedPerLine);
        uint32_t result = FxEmulate(nInst);

        exec_count++;
        uint16_t GSUStatus = Memory.FillRAM[0x3000 + GSU_SFR] | (Memory.FillRAM[0x3000 + GSU_SFR + 1] << 8);
        if ((GSUStatus & (FLG_G | FLG_IRQ)) == FLG_IRQ)
            S9xSetIRQ(GSU_IRQ_SOURCE);
    }
}

static void FxReset (struct FxInfo_s *psFxInfo)
{
    memset((uint8_t *) &GSU, 0, sizeof(struct FxRegs_s));

    GSU.pvSreg = GSU.pvDreg = &R0;

    GSU.pvRegisters       = psFxInfo->pvRegisters;
    GSU.nRamBanks         = psFxInfo->nRamBanks;
    GSU.pvRam             = psFxInfo->pvRam;
    GSU.nRomBanks         = psFxInfo->nRomBanks;
    GSU.pvRom             = psFxInfo->pvRom;
    GSU.vPrevScreenHeight = ~0;
    GSU.vPrevMode         = ~0;

    if (GSU.nRomBanks > 0x20)
        GSU.nRomBanks = 0x20;

    memset(GSU.pvRegisters, 0, 0x300);

    GSU.pvRegisters[0x3b] = 0;

    for (int i = 0; i < 256; i++)
    {
        uint32_t    b = i & 0x7f;

        if (b >= 0x40)
        {
            if (GSU.nRomBanks > 1)
                b %= GSU.nRomBanks;
            else
                b &= 1;

            GSU.apvRomBank[i] = &GSU.pvRom[b << 16];
        }
        else
        {
            b %= GSU.nRomBanks * 2;
            GSU.apvRomBank[i] = &GSU.pvRom[b << 16];
        }
    }

    for (int i = 0; i < 4; i++)
    {
        GSU.apvRamBank[i] = &GSU.pvRam[(i % GSU.nRamBanks) << 16];
        GSU.apvRomBank[0x70 + i] = GSU.apvRamBank[i];
    }

    GSU.vPipe = 0x01;

    GSU.pvCache = &GSU.pvRegisters[0x100];

    fx_readRegisterSpace();
}

static void fx_readRegisterSpace (void)
{
    static const uint32_t    avHeight[] = { 128, 160, 192, 256 };
    static const uint32_t    avMult[]   = {  16,  32,  32,  64 };

    uint8_t    *p;
    int        n;

    GSU.vErrorCode = 0;

    p = GSU.pvRegisters;
    for (int i = 0; i < 16; i++, p += 2)
        GSU.avReg[i] = (uint32_t) READ_WORD(p);

    p = GSU.pvRegisters;
    GSU.vStatusReg     =  (uint32_t) READ_WORD(&p[GSU_SFR]);
    GSU.vPrgBankReg    =  (uint32_t) p[GSU_PBR];
    GSU.vRomBankReg    =  (uint32_t) p[GSU_ROMBR];
    GSU.vRamBankReg    = ((uint32_t) p[GSU_RAMBR]) & (FX_RAM_BANKS - 1);
    GSU.vCacheBaseReg  =  (uint32_t) p[GSU_CBR];
    GSU.vCacheBaseReg |= ((uint32_t) p[GSU_CBR + 1]) << 8;

    GSU.vZero     = !(GSU.vStatusReg & FLG_Z);
    GSU.vSign     =  (GSU.vStatusReg & FLG_S)  << 12;
    GSU.vOverflow =  (GSU.vStatusReg & FLG_OV) << 16;
    GSU.vCarry    =  (GSU.vStatusReg & FLG_CY) >> 2;

    GSU.pvRamBank = GSU.apvRamBank[GSU.vRamBankReg & 0x3];
    GSU.pvRomBank = GSU.apvRomBank[GSU.vRomBankReg];
    GSU.pvPrgBank = GSU.apvRomBank[GSU.vPrgBankReg];

    GSU.pvScreenBase = &GSU.pvRam[USEX8(p[GSU_SCBR]) << 10];
    n  =  (int) (!!(p[GSU_SCMR] & 0x04));
    n |= ((int) (!!(p[GSU_SCMR] & 0x20))) << 1;
    GSU.vScreenHeight = GSU.vScreenRealHeight = avHeight[n];
    GSU.vMode = p[GSU_SCMR] & 0x03;

    if (n == 3)
        GSU.vScreenSize = (256 / 8) * (256 / 8) * 32;
    else
        GSU.vScreenSize = (GSU.vScreenHeight / 8) * (256 / 8) * avMult[GSU.vMode];

    if (GSU.vPlotOptionReg & 0x10)
        GSU.vScreenHeight = 256;

    if (GSU.pvScreenBase + GSU.vScreenSize > GSU.pvRam + (GSU.nRamBanks * 65536))
        GSU.pvScreenBase = GSU.pvRam + (GSU.nRamBanks * 65536) - GSU.vScreenSize;

    GSU.pfPlot = fx_PlotTable[GSU.vMode];
    GSU.pfRpix = fx_PlotTable[GSU.vMode + 5];

    fx_OpcodeTable[0x04c] = GSU.pfPlot;
    fx_OpcodeTable[0x14c] = GSU.pfRpix;
    fx_OpcodeTable[0x24c] = GSU.pfPlot;
    fx_OpcodeTable[0x34c] = GSU.pfRpix;

    fx_computeScreenPointers();
}

static void fx_writeRegisterSpace (void)
{
    uint8_t    *p;

    p = GSU.pvRegisters;
    for (int i = 0; i < 16; i++, p += 2)
        WRITE_WORD(p, GSU.avReg[i]);

    if (USEX16(GSU.vZero) == 0)
        SF(Z);
    else
        CF(Z);

    if (GSU.vSign & 0x8000)
        SF(S);
    else
        CF(S);

    if (GSU.vOverflow >= 0x8000 || GSU.vOverflow < -0x8000)
        SF(OV);
    else
        CF(OV);

    if (GSU.vCarry)
        SF(CY);
    else
        CF(CY);

    p = GSU.pvRegisters;
    WRITE_WORD(&p[GSU_SFR], GSU.vStatusReg);
    p[GSU_PBR]     = (uint8_t)  GSU.vPrgBankReg;
    p[GSU_ROMBR]   = (uint8_t)  GSU.vRomBankReg;
    p[GSU_RAMBR]   = (uint8_t)  GSU.vRamBankReg;
    WRITE_WORD(&p[GSU_CBR], GSU.vCacheBaseReg);
}

static void fx_updateRamBank (uint8_t byte)
{
    GSU.vRamBankReg = (uint32_t) byte & (FX_RAM_BANKS - 1);
    GSU.pvRamBank = GSU.apvRamBank[byte & 0x3];
}

static void fx_dirtySCBR (void)
{
    GSU.vSCBRDirty = true;
}

static bool fx_checkStartAddress (void)
{
    if (GSU.bCacheActive && R15 >= GSU.vCacheBaseReg && R15 < (GSU.vCacheBaseReg + 512))
        return true;


    if (SCMR & (1 << 4))
    {
        if (GSU.vPrgBankReg <= 0x5f || GSU.vPrgBankReg >= 0x80)
            return true;
    }

    if (GSU.vPrgBankReg <= 0x7f && (SCMR & (1 << 3)))
        return true;

    return false;
}

static uint32_t FxEmulate (uint32_t nInstructions)
{
    uint32_t    vCount;

    fx_readRegisterSpace();

    if (!fx_checkStartAddress())
    {
        CF(G);
        fx_writeRegisterSpace();

        return (0);
    }

    CF(IRQ);

    vCount = fx_run(nInstructions);

    fx_writeRegisterSpace();

    if (GSU.vErrorCode)
        return (GSU.vErrorCode);
    else
        return (vCount);
}

void fx_computeScreenPointers (void)
{
    if (GSU.vMode != GSU.vPrevMode || GSU.vPrevScreenHeight != GSU.vScreenHeight || GSU.vSCBRDirty)
    {
        GSU.vSCBRDirty = false;

        switch (GSU.vScreenHeight)
        {
            case 128:
                switch (GSU.vMode)
                {
                    case 0:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 4);
                            GSU.x[i] = i <<  8;
                        }

                        break;

                    case 1:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 5);
                            GSU.x[i] = i <<  9;
                        }

                        break;

                    case 2:
                    case 3:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 6);
                            GSU.x[i] = i << 10;
                        }

                        break;
                }

                break;

            case 160:
                switch (GSU.vMode)
                {
                    case 0:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 4);
                            GSU.x[i] = (i <<  8) + (i << 6);
                        }

                        break;

                    case 1:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 5);
                            GSU.x[i] = (i <<  9) + (i << 7);
                        }

                        break;

                    case 2:
                    case 3:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 6);
                            GSU.x[i] = (i << 10) + (i << 8);
                        }

                        break;
                }

                break;

            case 192:
                switch (GSU.vMode)
                {
                    case 0:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 4);
                            GSU.x[i] = (i <<  8) + (i << 7);
                        }

                        break;

                    case 1:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 5);
                            GSU.x[i] = (i <<  9) + (i << 8);
                        }

                        break;

                    case 2:
                    case 3:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + (i << 6);
                            GSU.x[i] = (i << 10) + (i << 9);
                        }

                        break;
                }

                break;

            case 256:
                switch (GSU.vMode)
                {
                    case 0:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + ((i & 0x10) <<  9) + ((i & 0xf) <<  8);
                            GSU.x[i] = ((i & 0x10) <<  8) + ((i & 0xf) << 4);
                        }

                        break;

                    case 1:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + ((i & 0x10) << 10) + ((i & 0xf) <<  9);
                            GSU.x[i] = ((i & 0x10) <<  9) + ((i & 0xf) << 5);
                        }

                        break;

                    case 2:
                    case 3:
                        for (int i = 0; i < 32; i++)
                        {
                            GSU.apvScreen[i] = GSU.pvScreenBase + ((i & 0x10) << 11) + ((i & 0xf) << 10);
                            GSU.x[i] = ((i & 0x10) << 10) + ((i & 0xf) << 6);
                        }

                        break;
                }

                break;
        }

        GSU.vPrevMode = GSU.vMode;
        GSU.vPrevScreenHeight = GSU.vScreenHeight;
    }
}

static void FxCacheWriteAccess (uint16_t vAddress)
{
    if ((vAddress & 0x00f) == 0x00f)
        GSU.vCacheFlags |= 1 << ((vAddress & 0x1f0) >> 4);
}

static void FxFlushCache (void)
{
    GSU.vCacheFlags = 0;
    GSU.vCacheBaseReg = 0;
    GSU.bCacheActive = false;
}

void fx_flushCache (void)
{
    GSU.vCacheFlags = 0;
    GSU.bCacheActive = false;
}
