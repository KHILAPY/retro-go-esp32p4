/*****************************************************************************
    Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
    For further information, consult the LICENSE file in the root directory.
******************************************************************************/

#ifndef _SDD1EMU_H_
#define _SDD1EMU_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void SDD1_decompress(uint8_t *out, uint8_t *in, int len);

#ifdef __cplusplus
}
#endif

#endif
