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
** map098.c: Mapper 98 (Namco 118) interface
** Used by: Final Fantasy 2 (J)
**
*/

#include "nes/nes.h"


static void map_write(uint32 address, uint8 value)
{
    switch (address & 0xE001)
    {
    case 0x8000:
        mmc_bankrom(16, 0x8000, value);
        break;

    case 0x8001:
        mmc_bankrom(16, 0xC000, value);
        break;

    case 0xA000:
        mmc_bankvrom(8, 0x0000, value);
        break;
    }
}

static void map_init(rom_t *cart)
{
    mmc_bankrom(16, 0x8000, 0);
    mmc_bankrom(16, 0xC000, MMC_LASTBANK);
    mmc_bankvrom(8, 0x0000, 0);
}


mapintf_t map98_intf =
{
    .number     = 98,
    .name       = "Namco 118",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = NULL,
    .set_state  = NULL,
    .mem_read   = {},
    .mem_write  = {
        { 0x8000, 0xFFFF, map_write }
    },
};
