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
** map252.c: Mapper 252 interface
** Used by: Sangokushi Genchuu Haisha (Chinese)
**
*/

#include "nes/nes.h"

static uint8 chr_bank[8];


static void map_write(uint32 address, uint8 value)
{
    if (address >= 0xB000 && address <= 0xBFFF)
    {
        uint8 reg = (address - 0xB000) >> 1;
        if (address & 1)
            chr_bank[reg] = (chr_bank[reg] & 0x0F) | (value << 4);
        else
            chr_bank[reg] = (chr_bank[reg] & 0xF0) | (value & 0x0F);
        mmc_bankvrom(1, reg * 0x400, chr_bank[reg]);
    }
    else if (address >= 0x8000 && address <= 0x8FFF)
    {
        mmc_bankrom(8, 0x8000, value);
    }
    else if (address >= 0x9000 && address <= 0x9FFF)
    {
        mmc_bankrom(8, 0xA000, value);
    }
    else if (address >= 0xA000 && address <= 0xAFFF)
    {
        mmc_bankrom(8, 0xC000, value);
    }
}

static void map_init(rom_t *cart)
{
    for (int i = 0; i < 8; i++)
        chr_bank[i] = 0;

    mmc_bankrom(8, 0x8000, 0);
    mmc_bankrom(8, 0xA000, 0);
    mmc_bankrom(8, 0xC000, 0);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
}


mapintf_t map252_intf =
{
    .number     = 252,
    .name       = "Mapper 252",
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
