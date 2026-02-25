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
** map091.c: Mapper 91 (HK-SF3) interface
** Used by: Street Fighter 3 (HK)
**
*/

#include "nes/nes.h"


static void map_write(uint32 address, uint8 value)
{
    switch (address & 0xF000)
    {
    case 0x6000:
        mmc_bankrom(8, 0x8000, value);
        break;

    case 0x7000:
        mmc_bankrom(8, 0xA000, value);
        break;

    case 0x8000:
    case 0x9000:
        mmc_bankvrom(2, 0x0000, value);
        break;

    case 0xA000:
    case 0xB000:
        mmc_bankvrom(2, 0x0800, value);
        break;

    case 0xC000:
    case 0xD000:
        mmc_bankvrom(2, 0x1000, value);
        break;

    case 0xE000:
    case 0xF000:
        mmc_bankvrom(2, 0x1800, value);
        break;
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


mapintf_t map91_intf =
{
    .number     = 91,
    .name       = "HK-SF3",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = NULL,
    .set_state  = NULL,
    .mem_read   = {},
    .mem_write  = {
        { 0x6000, 0xFFFF, map_write }
    },
};
