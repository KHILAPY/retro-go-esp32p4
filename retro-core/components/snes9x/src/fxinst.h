/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _FXINST_H_
#define _FXINST_H_

#include <stdint.h>
#include <stdbool.h>

#define FX_RAM_BANKS    4

#define FX_DO_ROMBUFFER

struct FxRegs_s
{
    uint32_t    avReg[16];
    uint32_t    vColorReg;
    uint32_t    vPlotOptionReg;
    uint32_t    vStatusReg;
    uint32_t    vPrgBankReg;
    uint32_t    vRomBankReg;
    uint32_t    vRamBankReg;
    uint32_t    vCacheBaseReg;
    uint32_t    vCacheFlags;
    uint32_t    vLastRamAdr;
    uint32_t    *pvDreg;
    uint32_t    *pvSreg;
    uint8_t     vRomBuffer;
    uint8_t     vPipe;
    uint32_t    vPipeAdr;

    uint32_t    vSign;
    uint32_t    vZero;
    uint32_t    vCarry;
    int32_t     vOverflow;

    int32_t     vErrorCode;
    uint32_t    vIllegalAddress;

    uint8_t     bBreakPoint;
    uint32_t    vBreakPoint;
    uint32_t    vStepPoint;

    uint8_t     *pvRegisters;
    uint32_t    nRamBanks;
    uint8_t     *pvRam;
    uint32_t    nRomBanks;
    uint8_t     *pvRom;

    uint32_t    vMode;
    uint32_t    vPrevMode;
    uint8_t     *pvScreenBase;
    uint8_t     *apvScreen[32];
    int32_t     x[32];
    uint32_t    vScreenHeight;
    uint32_t    vScreenRealHeight;
    uint32_t    vPrevScreenHeight;
    uint32_t    vScreenSize;
    void        (*pfPlot) (void);
    void        (*pfRpix) (void);

    uint8_t     *pvRamBank;
    uint8_t     *pvRomBank;
    uint8_t     *pvPrgBank;

    uint8_t     *apvRamBank[FX_RAM_BANKS];
    uint8_t     *apvRomBank[256];

    uint8_t     bCacheActive;
    uint8_t     *pvCache;
    uint8_t     avCacheBackup[512];
    uint32_t    vCounter;
    uint32_t    vInstCount;
    uint32_t    vSCBRDirty;

    uint8_t     *avRegAddr;
};

extern struct FxRegs_s  GSU;

#define GSU_R0          0x000
#define GSU_R1          0x002
#define GSU_R2          0x004
#define GSU_R3          0x006
#define GSU_R4          0x008
#define GSU_R5          0x00a
#define GSU_R6          0x00c
#define GSU_R7          0x00e
#define GSU_R8          0x010
#define GSU_R9          0x012
#define GSU_R10         0x014
#define GSU_R11         0x016
#define GSU_R12         0x018
#define GSU_R13         0x01a
#define GSU_R14         0x01c
#define GSU_R15         0x01e
#define GSU_SFR         0x030
#define GSU_BRAMR       0x033
#define GSU_PBR         0x034
#define GSU_ROMBR       0x036
#define GSU_CFGR        0x037
#define GSU_SCBR        0x038
#define GSU_CLSR        0x039
#define GSU_SCMR        0x03a
#define GSU_VCR         0x03b
#define GSU_RAMBR       0x03c
#define GSU_CBR         0x03e
#define GSU_CACHERAM    0x100

#define FLG_Z           (1 <<  1)
#define FLG_CY          (1 <<  2)
#define FLG_S           (1 <<  3)
#define FLG_OV          (1 <<  4)
#define FLG_G           (1 <<  5)
#define FLG_R           (1 <<  6)
#define FLG_ALT1        (1 <<  8)
#define FLG_ALT2        (1 <<  9)
#define FLG_IL          (1 << 10)
#define FLG_IH          (1 << 11)
#define FLG_B           (1 << 12)
#define FLG_IRQ         (1 << 15)

#define TF(a)           (GSU.vStatusReg &   FLG_##a)
#define CF(a)           (GSU.vStatusReg &= ~FLG_##a)
#define SF(a)           (GSU.vStatusReg |=  FLG_##a)

#define TS(a, b)        GSU.vStatusReg = ((GSU.vStatusReg & (~FLG_##a)) | ((!!(b)) * FLG_##a))

#define ALT0            (!TF(ALT1) && !TF(ALT2))
#define ALT1            ( TF(ALT1) && !TF(ALT2))
#define ALT2            (!TF(ALT1) &&  TF(ALT2))
#define ALT3            ( TF(ALT1) &&  TF(ALT2))

#define SEX8(a)         ((int32_t)  ((int8_t)   (a)))
#define SEX16(a)        ((int32_t)  ((int16_t)  (a)))

#define USEX8(a)        ((uint32_t) ((uint8_t)  (a)))
#define USEX16(a)       ((uint32_t) ((uint16_t) (a)))
#define SUSEX16(a)      ((int32_t)  ((uint16_t) (a)))

#define TSZ(num)        TS(S, ((num) & 0x8000)); TS(Z, (!USEX16(num)))

#define CLRFLAGS        GSU.vStatusReg &= ~(FLG_ALT1 | FLG_ALT2 | FLG_B); GSU.pvDreg = GSU.pvSreg = &R0

#define RAM(adr)        GSU.pvRamBank[USEX16(adr)]

#define ROM(idx)        GSU.pvRomBank[USEX16(idx)]

#define PIPE            GSU.vPipe

#define PRGBANK(idx)    GSU.pvPrgBank[USEX16(idx)]

#define FETCHPIPE       { PIPE = PRGBANK(R15); }

#ifndef ABS
#define ABS(x)          ((x) < 0 ? -(x) : (x))
#endif

#define SREG            (*GSU.pvSreg)

#define DREG            (*GSU.pvDreg)

#define READR14         GSU.vRomBuffer = ROM(R14)

#define TESTR14         if (GSU.pvDreg == &R14) READR14

#define R0              GSU.avReg[0]
#define R1              GSU.avReg[1]
#define R2              GSU.avReg[2]
#define R3              GSU.avReg[3]
#define R4              GSU.avReg[4]
#define R5              GSU.avReg[5]
#define R6              GSU.avReg[6]
#define R7              GSU.avReg[7]
#define R8              GSU.avReg[8]
#define R9              GSU.avReg[9]
#define R10             GSU.avReg[10]
#define R11             GSU.avReg[11]
#define R12             GSU.avReg[12]
#define R13             GSU.avReg[13]
#define R14             GSU.avReg[14]
#define R15             GSU.avReg[15]
#define SFR             GSU.vStatusReg
#define PBR             GSU.vPrgBankReg
#define ROMBR           GSU.vRomBankReg
#define RAMBR           GSU.vRamBankReg
#define CBR             GSU.vCacheBaseReg
#define SCBR            USEX8(GSU.pvRegisters[GSU_SCBR])
#define SCMR            USEX8(GSU.pvRegisters[GSU_SCMR])
#define COLR            GSU.vColorReg
#define POR             GSU.vPlotOptionReg
#define BRAMR           USEX8(GSU.pvRegisters[GSU_BRAMR])
#define VCR             USEX8(GSU.pvRegisters[GSU_VCR])
#define CFGR            USEX8(GSU.pvRegisters[GSU_CFGR])
#define CLSR            USEX8(GSU.pvRegisters[GSU_CLSR])

#define FX_STEP \
{ \
    uint32_t    vOpcode = (uint32_t) PIPE; \
    FETCHPIPE; \
    (*fx_OpcodeTable[(GSU.vStatusReg & 0x300) | vOpcode])(); \
}

extern void (*fx_PlotTable[]) (void);
extern void (*fx_OpcodeTable[]) (void);

#define BRANCH_DELAY_RELATIVE

#endif
