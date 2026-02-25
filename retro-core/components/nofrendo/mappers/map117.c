/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** map117.c: Mapper 117 (FS304) interface
** Used by: Sangokushi 4 (Chinese)
**
*/

#include "nes/nes.h"


static void map_write(uint32 address, uint8 value)
{
    if (address >= 0x8000 && address <= 0xBFFF)
    {
        uint8 reg = (address - 0x8000) >> 12;
        switch (reg)
        {
        case 0:
            mmc_bankrom(8, 0x8000, value);
            break;
        case 1:
            mmc_bankrom(8, 0xA000, value);
            break;
        case 2:
            mmc_bankrom(8, 0xC000, value);
            break;
        case 3:
            mmc_bankrom(8, 0xE000, value);
            break;
        }
    }
    else if (address >= 0xC000 && address <= 0xDFFF)
    {
        uint8 reg = address & 0x07;
        mmc_bankvrom(1, reg * 0x400, value);
    }
}

static void map_init(rom_t *cart)
{
    mmc_bankrom(8, 0x8000, 0);
    mmc_bankrom(8, 0xA000, 0);
    mmc_bankrom(8, 0xC000, 0);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
    mmc_bankvrom(8, 0x0000, 0);
}


mapintf_t map117_intf =
{
    .number     = 117,
    .name       = "FS304",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = NULL,
    .set_state  = NULL,
    .mem_read   = {},
    .mem_write  = {
        { 0x8000, 0xDFFF, map_write }
    },
};
