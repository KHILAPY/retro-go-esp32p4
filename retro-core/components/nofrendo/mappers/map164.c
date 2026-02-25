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
** map164.c: Mapper 164 interface
** Used by: Final Fantasy 3 (Chinese), Digital Monster
**
*/

#include "nes/nes.h"

static uint8 reg[2];


static void map_write(uint32 address, uint8 value)
{
    switch (address & 0x7300)
    {
    case 0x5100:
        reg[0] = value;
        break;

    case 0x5200:
        reg[1] = value;
        mmc_bankrom(32, 0x8000, (reg[0] << 4) | reg[1]);
        break;
    }
}

static void map_getstate(uint8 *state)
{
    state[0] = reg[0];
    state[1] = reg[1];
}

static void map_setstate(uint8 *state)
{
    reg[0] = state[0];
    reg[1] = state[1];
    mmc_bankrom(32, 0x8000, (reg[0] << 4) | reg[1]);
}

static void map_init(rom_t *cart)
{
    reg[0] = 0;
    reg[1] = 0;

    mmc_bankrom(32, 0x8000, 0);
}


mapintf_t map164_intf =
{
    .number     = 164,
    .name       = "Mapper 164",
    .init       = map_init,
    .vblank     = NULL,
    .hblank     = NULL,
    .get_state  = map_getstate,
    .set_state  = map_setstate,
    .mem_read   = {},
    .mem_write  = {
        { 0x5000, 0x5FFF, map_write }
    },
};
