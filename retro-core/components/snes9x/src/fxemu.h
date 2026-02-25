/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _FXEMU_H_
#define _FXEMU_H_

#include <stdint.h>
#include <stdbool.h>

#define FX_BREAKPOINT               (-1)
#define FX_ERROR_ILLEGAL_ADDRESS    (-2)

struct FxInfo_s
{
    uint32_t    vFlags;
    uint8_t     *pvRegisters;
    uint32_t    nRamBanks;
    uint8_t     *pvRam;
    uint32_t    nRomBanks;
    uint8_t     *pvRom;
    uint32_t    speedPerLine;
    bool        oneLineDone;
};

extern struct FxInfo_s    SuperFX;

void S9xInitSuperFX (void);
void S9xResetSuperFX (void);
void S9xSuperFXExec (void);
void S9xSetSuperFX (uint8_t, uint16_t);
uint8_t S9xGetSuperFX (uint16_t);
void fx_flushCache (void);
void fx_computeScreenPointers (void);
uint32_t fx_run (uint32_t);

#endif
