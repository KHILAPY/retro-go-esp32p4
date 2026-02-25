/*****************************************************************************
    Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
    For further information, consult the LICENSE file in the root directory.
******************************************************************************/

#ifndef _SDD1_H_
#define _SDD1_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void S9xSetSDD1MemoryMap(uint32_t bank, uint32_t value);
void S9xResetSDD1(void);
void S9xSDD1PostLoadState(void);

#ifdef __cplusplus
}
#endif

#endif
