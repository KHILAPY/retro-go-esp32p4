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
** map082.c: Mapper 82 (Taito X117) interface
** Used by: Kyuukyoku Stadium, Kyuukyoku Harikiri Stadium series
**
*/

#include "nes/nes.h"

static uint8 command;


static void map_write(uint32 address, uint8 value)
{
    switch (address & 0xF000)
    {
    case 0x7000:
    case 0x8000:
    case 0x9000:
    case 0xA000:
        command = value & 0x0F;
        break;

    case 0xB000:
        switch (command)
        {
        case 0:
            mmc_bankvrom(1, 0x0000, value);
            break;

        case 1:
            mmc_bankvrom(1, 0x0400, value);
            break;

        case 2:
            mmc_bankvrom(1, 0x0800, value);
            break;

        case 3:
            mmc_bankvrom(1, 0x0C00, value);
            break;

        case 4:
            mmc_bankvrom(1, 0x1000, value);
            break;

        case 5:
            mmc_bankvrom(1, 0x1400, value);
            break;

        case 6:
            mmc_bankvrom(1, 0x1800, value);
            break;

        case 7:
            mmc_bankvrom(1, 0x1C00, value);
            break;

        case 8:
            mmc_bankrom(8, 0x8000, value);
            break;

        case 9:
            mmc_bankrom(8, 0xA000, value);
            break;

        case 10:
            mmc_bankrom(8, 0xC000, value);
            break;

        case 11:
            mmc_bankrom(8, 0xE000, value);
            break;

        case 12:
            if (value & 1)
                ppu_setmirroring(PPU_MIRROR_HORI);
            else
                ppu_setmirroring(PPU_MIRROR_VERT);
            break;
        }
        break;
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = command;
}

static void map_setstate(uint8 *state)
{
    command = state[0];
}

static void map_init(rom_t *cart)
{
    command = 0;

    mmc_bankrom(8, 0x8000, 0);
    mmc_bankrom(8, 0xA000, 0);
    mmc_bankrom(8, 0xC000, 0);
    mmc_bankrom(8, 0xE000, MMC_LASTBANK);
}


mapintf_t map82_intf =
{
    .number     = 82,
    .name       = "Taito X117",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x7000, 0xBFFF, map_write }
    },
};
